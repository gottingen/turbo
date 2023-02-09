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
//
// -----------------------------------------------------------------------------
// File: log/internal/test_matchers.h
// -----------------------------------------------------------------------------
//
// This file declares Googletest's matchers used in the Turbo Logging library
// unit tests.

#ifndef TURBO_LOG_INTERNAL_TEST_MATCHERS_H_
#define TURBO_LOG_INTERNAL_TEST_MATCHERS_H_

#include <iosfwd>
#include <sstream>
#include <string>

#include "turbo/base/log_severity.h"
#include "turbo/log/internal/test_helpers.h"
#include "turbo/log/log_entry.h"
#include "turbo/platform/port.h"
#include "turbo/strings/string_view.h"
#include "turbo/time/time.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {
// In some configurations, Googletest's string matchers (e.g.
// `::testing::EndsWith`) need help to match `turbo::string_view`.
::testing::Matcher<turbo::string_view> AsString(
    const ::testing::Matcher<const std::string&>& str_matcher);

// These matchers correspond to the components of `turbo::LogEntry`.
::testing::Matcher<const turbo::LogEntry&> SourceFilename(
    const ::testing::Matcher<turbo::string_view>& source_filename);
::testing::Matcher<const turbo::LogEntry&> SourceBasename(
    const ::testing::Matcher<turbo::string_view>& source_basename);
// Be careful with this one; multi-line statements using `__LINE__` evaluate
// differently on different platforms.  In particular, the MSVC implementation
// of `EXPECT_DEATH` returns the line number of the macro expansion to all lines
// within the code block that's expected to die.
::testing::Matcher<const turbo::LogEntry&> SourceLine(
    const ::testing::Matcher<int>& source_line);
::testing::Matcher<const turbo::LogEntry&> Prefix(
    const ::testing::Matcher<bool>& prefix);
::testing::Matcher<const turbo::LogEntry&> LogSeverity(
    const ::testing::Matcher<turbo::LogSeverity>& log_severity);
::testing::Matcher<const turbo::LogEntry&> Timestamp(
    const ::testing::Matcher<turbo::Time>& timestamp);
// Matches if the `LogEntry`'s timestamp falls after the instantiation of this
// matcher and before its execution, as is normal when used with EXPECT_CALL.
::testing::Matcher<const turbo::LogEntry&> TimestampInMatchWindow();
::testing::Matcher<const turbo::LogEntry&> ThreadID(
    const ::testing::Matcher<turbo::LogEntry::tid_t>&);
::testing::Matcher<const turbo::LogEntry&> TextMessageWithPrefixAndNewline(
    const ::testing::Matcher<turbo::string_view>&
        text_message_with_prefix_and_newline);
::testing::Matcher<const turbo::LogEntry&> TextMessageWithPrefix(
    const ::testing::Matcher<turbo::string_view>& text_message_with_prefix);
::testing::Matcher<const turbo::LogEntry&> TextMessage(
    const ::testing::Matcher<turbo::string_view>& text_message);
::testing::Matcher<const turbo::LogEntry&> TextPrefix(
    const ::testing::Matcher<turbo::string_view>& text_prefix);
::testing::Matcher<const turbo::LogEntry&> Verbosity(
    const ::testing::Matcher<int>& verbosity);
::testing::Matcher<const turbo::LogEntry&> Stacktrace(
    const ::testing::Matcher<turbo::string_view>& stacktrace);
// Behaves as `Eq(stream.str())`, but produces better failure messages.
::testing::Matcher<turbo::string_view> MatchesOstream(
    const std::ostringstream& stream);
::testing::Matcher<const std::string&> DeathTestValidateExpectations();

::testing::Matcher<const turbo::LogEntry&> RawEncodedMessage(
    const ::testing::Matcher<turbo::string_view>& raw_encoded_message);
#define ENCODED_MESSAGE(message_matcher) ::testing::_

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_TEST_MATCHERS_H_
