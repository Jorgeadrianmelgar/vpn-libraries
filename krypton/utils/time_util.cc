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

#include "privacy/net/krypton/utils/time_util.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "privacy/net/krypton/utils/status.h"
#include "third_party/absl/time/clock.h"
#include "third_party/absl/time/time.h"

namespace privacy {
namespace krypton {
namespace utils {
namespace {

const uint32_t kLatencyCollectionLimit = 5;

// Validation requirements documented in duration.proto.
inline ::absl::Status ValidateDuration(const google::protobuf::Duration& d) {
  const auto sec = d.seconds();
  const auto ns = d.nanos();
  if (sec < -315576000000 || sec > 315576000000) {
    return ::absl::InvalidArgumentError(absl::StrCat("seconds=", sec));
  }
  if (ns < -999999999 || ns > 999999999) {
    return ::absl::InvalidArgumentError(absl::StrCat("nanos=", ns));
  }
  if ((sec < 0 && ns > 0) || (sec > 0 && ns < 0)) {
    return ::absl::InvalidArgumentError("sign mismatch");
  }
  return ::absl::OkStatus();
}

}  // namespace

absl::Status ToProtoDuration(absl::Duration d,
                             google::protobuf::Duration* proto) {
  // s and n may both be negative, per the Duration proto spec.
  const int64_t s = absl::IDivDuration(d, absl::Seconds(1), &d);
  const int64_t n = absl::IDivDuration(d, absl::Nanoseconds(1), &d);
  proto->set_seconds(s);
  proto->set_nanos(n);
  PPN_RETURN_IF_ERROR(ValidateDuration(*proto));
  return absl::OkStatus();
}

absl::Status ToProtoTime(absl::Time t, google::protobuf::Timestamp* proto) {
  // A Timestamp is stored as a duration since epoch.
  absl::Duration since_epoch = t - absl::UnixEpoch();
  google::protobuf::Duration duration_proto;
  PPN_RETURN_IF_ERROR(ToProtoDuration(since_epoch, &duration_proto));

  proto->set_seconds(duration_proto.seconds());
  proto->set_nanos(duration_proto.nanos());
  return absl::OkStatus();
}

absl::StatusOr<absl::Duration> DurationFromProto(
    const google::protobuf::Duration& proto) {
  PPN_RETURN_IF_ERROR(ValidateDuration(proto));
  return absl::Seconds(proto.seconds()) + absl::Nanoseconds(proto.nanos());
}

absl::StatusOr<absl::Time> TimeFromProto(
    const google::protobuf::Timestamp& proto) {
  return absl::FromUnixSeconds(proto.seconds()) +
         absl::Nanoseconds(proto.nanos());
}

absl::StatusOr<absl::Time> ParseTimestamp(absl::string_view s) {
  absl::Time time;
  std::string error;
  if (!absl::ParseTime(absl::RFC3339_full, s, &time, &error)) {
    LOG(ERROR) << "Unable to parse timestamp [" << s << "]";
    return absl::InvalidArgumentError(
        absl::StrCat("Unable to parse timestamp [", s, "]"));
  }
  return time;
}

absl::Status VerifyTimestampIsRounded(
    const google::protobuf::Timestamp& timestamp, absl::Duration increments) {
  if (timestamp.nanos() != 0 ||
      timestamp.seconds() % (absl::ToInt64Seconds(increments)) != 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("Expiry timestamp not in increments of ",
                     absl::FormatDuration(increments)));
  }
  return absl::OkStatus();
}

void RecordLatency(absl::Time& start,
                   std::vector<google::protobuf::Duration>* latencies,
                   std::string_view latency_type) {
  google::protobuf::Duration latency;
  absl::Duration latency_durition = absl::Now() - start;
  auto latency_status = utils::ToProtoDuration(latency_durition, &latency);
  if (!latency_status.ok()) {
    LOG(ERROR) << "Unable to calculate " << latency_type
               << " latency with status:" << latency_status;
    return;
  }
  if (latencies->size() >= kLatencyCollectionLimit) {
    LOG(ERROR) << "Max " << latency_type
               << " latency collection limit reached, not adding latency:"
               << latency_durition;
    return;
  }
  latencies->emplace_back(latency);
  start = absl::InfinitePast();
}

}  // namespace utils
}  // namespace krypton
}  // namespace privacy
