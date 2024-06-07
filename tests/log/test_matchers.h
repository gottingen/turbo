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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <tests/log/test_helpers.h>
#include <turbo/log/log_entry.h>
#include <turbo/strings/string_view.h>
#include <turbo/times/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {
// In some configurations, Googletest's string matchers (e.g.
// `::testing::ends_with`) need help to match `std::string_view`.
::testing::Matcher<std::string_view> AsString(
    const ::testing::Matcher<const std::string&>& str_matcher);

// These matchers correspond to the components of `turbo::LogEntry`.
::testing::Matcher<const turbo::LogEntry&> SourceFilename(
    const ::testing::Matcher<std::string_view>& source_filename);
::testing::Matcher<const turbo::LogEntry&> SourceBasename(
    const ::testing::Matcher<std::string_view>& source_basename);
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
    const ::testing::Matcher<std::string_view>&
        text_message_with_prefix_and_newline);
::testing::Matcher<const turbo::LogEntry&> TextMessageWithPrefix(
    const ::testing::Matcher<std::string_view>& text_message_with_prefix);
::testing::Matcher<const turbo::LogEntry&> TextMessage(
    const ::testing::Matcher<std::string_view>& text_message);
::testing::Matcher<const turbo::LogEntry&> TextPrefix(
    const ::testing::Matcher<std::string_view>& text_prefix);
::testing::Matcher<const turbo::LogEntry&> Verbosity(
    const ::testing::Matcher<int>& verbosity);
::testing::Matcher<const turbo::LogEntry&> Stacktrace(
    const ::testing::Matcher<std::string_view>& stacktrace);
// Behaves as `Eq(stream.str())`, but produces better failure messages.
::testing::Matcher<std::string_view> MatchesOstream(
    const std::ostringstream& stream);
::testing::Matcher<const std::string&> DeathTestValidateExpectations();

::testing::Matcher<const turbo::LogEntry&> RawEncodedMessage(
    const ::testing::Matcher<std::string_view>& raw_encoded_message);
#define ENCODED_MESSAGE(message_matcher) ::testing::_

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_TEST_MATCHERS_H_
