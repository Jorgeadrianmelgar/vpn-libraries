// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "privacy/net/krypton/timer_manager.h"

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <utility>

#include "base/logging.h"
#include "privacy/net/krypton/pal/timer_interface.h"
#include "third_party/absl/functional/bind_front.h"
#include "third_party/absl/status/status.h"
#include "third_party/absl/status/statusor.h"
#include "third_party/absl/synchronization/mutex.h"
#include "third_party/absl/time/time.h"

namespace privacy {
namespace krypton {
namespace {
// Increments for the timer id.
constexpr int kTimerIncrementValue = 1;
}  // namespace

TimerManager::TimerManager(TimerInterface* timer_interface)
    : timer_interface_(timer_interface) {
  // We register the callback to receive timer expiry call backs.
  timer_interface_->RegisterCallback(
      absl::bind_front(&TimerManager::TimerExpiry, this));
}

TimerManager::~TimerManager() {
  absl::MutexLock l(&mutex_);
  timer_map_.clear();
}

absl::StatusOr<int> TimerManager::StartTimer(absl::Duration duration,
                                             TimerCb callback,
                                             absl::string_view label) {
  absl::MutexLock l(&mutex_);
  auto timer_id = timer_id_counter_.fetch_add(kTimerIncrementValue);

  auto timer_start_status = timer_interface_->StartTimer(timer_id, duration);
  if (!timer_start_status.ok()) {
    return timer_start_status;
  }
  LOG(INFO) << "Starting timer " << label << " of " << duration
            << " with id: " << timer_id;
  TimerDetails timer_details;
  timer_details.timer_cb = std::move(callback);
  timer_details.label.assign(label.data(), label.size());
  timer_map_.emplace(timer_id, std::move(timer_details));
  return timer_id;
}

void TimerManager::CancelTimer(int timer_id) {
  LOG(INFO) << "Cancelling timer with id: " << timer_id;
  absl::MutexLock l(&mutex_);
  timer_interface_->CancelTimer(timer_id);
  auto result = timer_map_.find(timer_id);
  if (result != timer_map_.end()) {
    LOG(INFO) << "Cancelled timer " << result->second.label
              << " with id: " << timer_id;
    timer_map_.erase(result);
  } else {
    LOG(WARNING) << "Cancelled unknown timer with id: " << timer_id;
  }
}

void TimerManager::TimerExpiry(int timer_id) {
  TimerCb timer_cb = nullptr;
  std::string label;
  {
    absl::MutexLock l(&mutex_);
    auto result = timer_map_.find(timer_id);
    if (result != timer_map_.end()) {
      timer_cb = result->second.timer_cb;
      label = result->second.label;
      timer_map_.erase(result);
    } else {
      LOG(ERROR) << "Timer expiry for timer_id: " << timer_id
                 << " that is not running";
      return;
    }
  }
  if (timer_cb != nullptr) {
    LOG(INFO) << "Calling callback for timer " << label
              << " with id: " << timer_id;
    timer_cb();
  }
}

}  // namespace krypton
}  // namespace privacy
