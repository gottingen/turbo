//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <turbo/log/structured.h>

#include <ios>
#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <tests/log/test_helpers.h>
#include <tests/log/test_matchers.h>
#include <turbo/log/log.h>
#include <tests/log/scoped_mock_log.h>

namespace {
using ::turbo::log_internal::MatchesOstream;
using ::turbo::log_internal::TextMessage;
using ::testing::Eq;

auto *test_env TURBO_ATTRIBUTE_UNUSED = ::testing::AddGlobalTestEnvironment(
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
