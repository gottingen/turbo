//
// Copyright 2022 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/log/structured.h"

#include <ios>
#include <sstream>
#include <string>

#include "turbo/log/internal/test_helpers.h"
#include "turbo/log/internal/test_matchers.h"
#include "turbo/log/log.h"
#include "turbo/log/scoped_mock_log.h"
#include "turbo/platform/port.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
using ::turbo::log_internal::MatchesOstream;
using ::turbo::log_internal::TextMessage;
using ::testing::Eq;

auto *test_env TURBO_MAYBE_UNUSED = ::testing::AddGlobalTestEnvironment(
    new turbo::log_internal::LogTestEnvironment);

// Turbo Logging library uses these by default, so we set them on the
// `std::ostream` we compare against too.
std::ios &LoggingDefaults(std::ios &str) {
  str.setf(std::ios_base::showbase | std::ios_base::boolalpha |
           std::ios_base::internal);
  return str;
}

TEST(StreamingFormatTest, LogAsLiteral) {
  std::ostringstream stream;
  const std::string not_a_literal("hello world");
  stream << LoggingDefaults << turbo::LogAsLiteral(not_a_literal);

  turbo::ScopedMockLog sink;

  EXPECT_CALL(sink,
              Send(AllOf(TextMessage(MatchesOstream(stream)),
                         TextMessage(Eq("hello world")),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { literal: "hello world" })pb")))));

  sink.StartCapturingLogs();
  LOG(INFO) << turbo::LogAsLiteral(not_a_literal);
}

}  // namespace
