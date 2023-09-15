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

#include <vector>

#include "google/protobuf/duration.proto.h"
#include "google/protobuf/timestamp.proto.h"
#include "testing/base/public/gmock.h"
#include "testing/base/public/gunit.h"
#include "third_party/absl/status/status.h"
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/strings/string_view.h"
#include "third_party/absl/time/clock.h"
#include "third_party/absl/time/time.h"
#include "third_party/absl/types/optional.h"

namespace privacy {
namespace krypton {
namespace utils {
namespace {

using ::testing::EqualsProto;
using ::testing::status::IsOkAndHolds;

TEST(Time, TestDurationFromProtoGood) {
  google::protobuf::Duration proto;
  proto.set_seconds(42);
  proto.set_nanos(0);
  ASSERT_OK_AND_ASSIGN(auto duration, DurationFromProto(proto));
  EXPECT_EQ(duration, absl::Seconds(42));

  proto.set_seconds(0);
  proto.set_nanos(120);
  ASSERT_OK_AND_ASSIGN(duration, DurationFromProto(proto));
  EXPECT_EQ(duration, absl::Nanoseconds(120));
}

TEST(Time, TestDurationFromProtoBad) {
  google::protobuf::Duration proto;
  proto.set_seconds(0);
  proto.set_nanos(1000000000);
  EXPECT_THAT(DurationFromProto(proto),
              testing::status::StatusIs(absl::StatusCode::kInvalidArgument));

  // The max value of seconds is documented in duration.proto as 315,576,000,000
  // which is about 10,000 years
  proto.set_seconds(315576000001);
  proto.set_nanos(0);
  EXPECT_THAT(DurationFromProto(proto),
              testing::status::StatusIs(absl::StatusCode::kInvalidArgument));

  proto.set_seconds(1);
  proto.set_nanos(-120);
  EXPECT_THAT(DurationFromProto(proto),
              testing::status::StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(Time, TestDurationToProto) {
  google::protobuf::Duration proto;
  EXPECT_OK(ToProtoDuration(absl::Seconds(42), &proto));
  EXPECT_THAT(proto, EqualsProto(R"pb(
                seconds: 42 nanos: 0
              )pb"));

  EXPECT_OK(ToProtoDuration(absl::Milliseconds(43044), &proto));
  EXPECT_THAT(proto, EqualsProto(R"pb(
                seconds: 43 nanos: 44000000
              )pb"));

  EXPECT_OK(ToProtoDuration(absl::Nanoseconds(45046047048), &proto));
  EXPECT_THAT(proto, EqualsProto(R"pb(
                seconds: 45 nanos: 46047048
              )pb"));
}

TEST(Time, TestTimestampToProto) {
  google::protobuf::Timestamp proto;
  EXPECT_OK(ToProtoTime(absl::FromUnixSeconds(1596762373), &proto));
  EXPECT_THAT(proto, EqualsProto(R"pb(
                seconds: 1596762373 nanos: 0
              )pb"));

  EXPECT_OK(ToProtoTime(absl::FromUnixMillis(1596762373123L), &proto));
  EXPECT_THAT(proto, EqualsProto(R"pb(
                seconds: 1596762373 nanos: 123000000
              )pb"));
}

TEST(Time, TestParseTimestamp) {
  EXPECT_THAT(ParseTimestamp("2020-08-07T01:06:13+00:00"),
              IsOkAndHolds(absl::FromUnixSeconds(1596762373)));
}

TEST(Time, TestTimeFromProto) {
  // Time used maps to: 2020-02-13T11:31:30+00:00".
  google::protobuf::Timestamp timestamp;
  timestamp.set_seconds(1234567890);
  timestamp.set_nanos(12345);
  absl::StatusOr<absl::Time> time = TimeFromProto(timestamp);
  ASSERT_OK(time);
  ASSERT_EQ(time.value(), absl::FromUnixNanos(1234567890000012345));
}

TEST(Time, VerifyTimestampIncrements) {
  google::protobuf::Timestamp expiry_time;
  absl::Duration increments = absl::Minutes(15);
  // (GMT): February 1, 2023 6:19:00 AM
  auto time = 1675232340000;
  expiry_time.set_seconds(time);
  expiry_time.set_nanos(0);
  EXPECT_EQ(VerifyTimestampIsRounded(expiry_time, increments),
            absl::InvalidArgumentError(
                absl::StrCat("Expiry timestamp not in increments of ",
                             absl::FormatDuration(increments))));
  // (GMT): February 1, 2023 6:15:00 AM
  time = 1675232100000;
  expiry_time.clear_seconds();
  expiry_time.set_seconds(time);
  EXPECT_EQ(VerifyTimestampIsRounded(expiry_time, increments),
            absl::OkStatus());
  // check nanos as well
  expiry_time.clear_nanos();
  expiry_time.set_nanos(123);
  EXPECT_EQ(VerifyTimestampIsRounded(expiry_time, increments),
            absl::InvalidArgumentError(
                absl::StrCat("Expiry timestamp not in increments of ",
                             absl::FormatDuration(increments))));
}

TEST(Time, RecordLatencyResetsStart) {
  absl::Time start = absl::Now();
  std::vector<google::protobuf::Duration> latencies;
  RecordLatency(start, &latencies, "test_latency");
  EXPECT_EQ(start, absl::InfinitePast());
}

TEST(Time, RecordLatencyVectorSize) {
  absl::Time start = absl::Now();
  std::vector<google::protobuf::Duration> latencies;
  RecordLatency(start, &latencies, "test_latency");
  start = absl::Now();
  RecordLatency(start, &latencies, "test_latency");
  EXPECT_EQ(latencies.size(), 2);

  // Test the kLatencyCollectionLimit
  for (int i = 0; i < 6; i++) {
    start = absl::Now();
    RecordLatency(start, &latencies, "test_latency");
  }
  EXPECT_EQ(latencies.size(), 5);
}

TEST(Time, RecordLatencyBadLatencyStatus) {
  // A latency with InfinitePast() start time won't have an OK latency_status.
  // Latency shouldn't be recorded if latency_status != ok.
  absl::Time start = absl::InfinitePast();
  std::vector<google::protobuf::Duration> latencies;
  RecordLatency(start, &latencies, "test_latency");
  EXPECT_EQ(latencies.size(), 0);
}

}  // namespace
}  // namespace utils
}  // namespace krypton
}  // namespace privacy
