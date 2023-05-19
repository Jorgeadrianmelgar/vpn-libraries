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

#include "privacy/net/krypton/utils/ip_range.h"

#include <sys/socket.h>

#include <optional>
#include <string>

#include "privacy/net/krypton/proto/tun_fd_data.proto.h"
#include "testing/base/public/gmock.h"
#include "testing/base/public/gunit.h"
#include "third_party/absl/status/status.h"

namespace privacy {
namespace krypton {
namespace utils {
namespace {

using ::testing::Eq;
using ::testing::Optional;
using ::testing::status::StatusIs;

TEST(IPRange, TestGetPortFromHostPort) {
  std::string host;
  std::string port;

  // Bare port.
  EXPECT_OK(ParseHostPort(":12345", &host, &port));
  EXPECT_EQ(host, "");
  EXPECT_EQ(port, "12345");
}

TEST(IPRange, TestGetDomainAndPortFromHostPort) {
  std::string host;
  std::string port;

  // Domains.
  EXPECT_OK(ParseHostPort("example.com:12345", &host, &port));
  EXPECT_EQ(host, "example.com");
  EXPECT_EQ(port, "12345");
}

TEST(IPRange, TestGetIP4AddressAndPortFromHostPort) {
  std::string host;
  std::string port;

  // IPv4
  EXPECT_OK(ParseHostPort("127.0.0.1:12345", &host, &port));
  EXPECT_EQ(host, "127.0.0.1");
  EXPECT_EQ(port, "12345");
}

TEST(IPRange, TestGetIP6AddressAndPortFromHostPort) {
  std::string host;
  std::string port;

  // IPv6
  EXPECT_OK(ParseHostPort("[2604:fe::03]:12345", &host, &port));
  EXPECT_EQ(host, "2604:fe::03");
  EXPECT_EQ(port, "12345");
}

TEST(IPRange, TestGetBareIP6AddressAndPortFromHostPort) {
  std::string host;
  std::string port;

  // Bare IPv6
  EXPECT_OK(ParseHostPort("2604:fe::3", &host, &port));
  EXPECT_EQ(host, "2604:fe::3");
  EXPECT_EQ(port, "");
}

// Testing V4
TEST(IPRange, TestValidIPv4WithPrefix) {
  ASSERT_OK_AND_ASSIGN(const IPRange ip_range, IPRange::Parse("10.2.2.32/32"));
  EXPECT_EQ(ip_range.address(), "10.2.2.32");
  EXPECT_THAT(ip_range.prefix(), Optional(Eq(32)));
  EXPECT_EQ(ip_range.family(), AF_INET);
}

TEST(IPRange, TestValidIPv4WithoutPrefix) {
  ASSERT_OK_AND_ASSIGN(const IPRange ip_range, IPRange::Parse("10.2.2.32"));
  EXPECT_EQ(ip_range.address(), "10.2.2.32");
  EXPECT_THAT(ip_range.prefix(), Eq(std::nullopt));
  EXPECT_EQ(ip_range.family(), AF_INET);
}

TEST(IPRange, TestIncompleteIP) {
  EXPECT_THAT(IPRange::Parse("10.2.2"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(IPRange, TestInvalidSyntax) {
  EXPECT_THAT(IPRange::Parse("10.2.2.32/32/abc"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(IPRange, TestInvalidRange) {
  EXPECT_THAT(IPRange::Parse("10.2.2.32/ab"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IPRange::Parse("10.2.2.32/-12"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IPRange::Parse("10.2.2.32/64"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

// Testing V6
TEST(IPRange, TestValidIPv6WithPrefix) {
  ASSERT_OK_AND_ASSIGN(const IPRange ip_range,
                       IPRange::Parse("2604:fe::03/64"));
  EXPECT_EQ(ip_range.address(), "2604:fe::03");
  EXPECT_THAT(ip_range.prefix(), Optional(Eq(64)));
  EXPECT_EQ(ip_range.family(), AF_INET6);
}

TEST(IPRange, TestValidIPv6WithoutPrefix) {
  ASSERT_OK_AND_ASSIGN(const IPRange ip_range, IPRange::Parse("2604:fe::03"));
  EXPECT_EQ(ip_range.address(), "2604:fe::03");
  EXPECT_THAT(ip_range.prefix(), Eq(std::nullopt));
  EXPECT_EQ(ip_range.family(), AF_INET6);
}

TEST(IPRange, TestIncompleteIPv6) {
  EXPECT_THAT(IPRange::Parse("2604:fe"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(IPRange, TestInvalidV6Syntax) {
  EXPECT_THAT(IPRange::Parse("2604:fe::03/32/abc"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(IPRange, TestInvalidV6Range) {
  EXPECT_THAT(IPRange::Parse("2604:fe::03/ab"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IPRange::Parse("2604:fe::03/-12"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IPRange::Parse("2604:fe::03/256"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

// Testing proto conversion.
TEST(IPRange, TestFromProtoInvalidFamily) {
  TunFdData::IpRange proto;
  EXPECT_THAT(IPRange::FromProto(proto),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(IPRange, TestFromProtoIPv4WithPrefix) {
  TunFdData::IpRange proto;
  proto.set_ip_family(TunFdData::IpRange::IPV4);
  proto.set_ip_range("127.0.0.1");
  proto.set_prefix(32);

  // TODO: Change this to an OSS-friendly pattern once one is
  // available.
  ASSERT_OK_AND_ASSIGN(const IPRange range, IPRange::FromProto(proto));
  EXPECT_EQ(range.address(), "127.0.0.1");
  EXPECT_EQ(range.prefix(), 32);
  EXPECT_EQ(range.family(), AF_INET);
}

TEST(IPRange, TestFromProtoIPv4WithoutPrefix) {
  TunFdData::IpRange proto;
  proto.set_ip_family(TunFdData::IpRange::IPV4);
  proto.set_ip_range("127.0.0.1");

  ASSERT_OK_AND_ASSIGN(const IPRange range, IPRange::FromProto(proto));
  EXPECT_EQ(range.address(), "127.0.0.1");
  EXPECT_EQ(range.prefix(), std::nullopt);
  EXPECT_EQ(range.family(), AF_INET);
}

TEST(IPRange, TestFromProtoIPv6) {
  TunFdData::IpRange proto;
  proto.set_ip_family(TunFdData::IpRange::IPV6);
  proto.set_ip_range("2604:fe::03");
  proto.set_prefix(64);

  ASSERT_OK_AND_ASSIGN(const IPRange range, IPRange::FromProto(proto));
  EXPECT_EQ(range.address(), "2604:fe::03");
  EXPECT_EQ(range.prefix(), 64);
  EXPECT_EQ(range.family(), AF_INET6);
}

TEST(IPRange, TestResolveIPAddress) {
  ASSERT_OK_AND_ASSIGN(auto ip, ResolveIPAddress("localhost"));
  EXPECT_EQ(ip.compare("::1"), 0);
}

}  // namespace
}  // namespace utils
}  // namespace krypton
}  // namespace privacy
