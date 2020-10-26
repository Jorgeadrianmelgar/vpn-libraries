// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the );
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an  BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "privacy/net/krypton/utils/time_util.h"

namespace privacy {
namespace krypton {
namespace utils {
namespace {

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
  const int64 s = absl::IDivDuration(d, absl::Seconds(1), &d);
  const int64 n = absl::IDivDuration(d, absl::Nanoseconds(1), &d);
  proto->set_seconds(s);
  proto->set_nanos(n);
  PPN_RETURN_IF_ERROR(ValidateDuration(*proto));
  return absl::OkStatus();
}

}  // namespace utils
}  // namespace krypton
}  // namespace privacy
