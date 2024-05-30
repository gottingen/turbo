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

#include <tests/log/test_matchers.h>

#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <tests/log/test_helpers.h>
#include <turbo/strings/string_view.h>
//#include <turbo/times/clock.h>
#include <turbo/times/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {
namespace {
using ::testing::_;
using ::testing::AllOf;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Not;
using ::testing::Property;
using ::testing::ResultOf;
using ::testing::Truly;

class AsStringImpl final
    : public MatcherInterface<turbo::string_view> {
 public:
  explicit AsStringImpl(
      const Matcher<const std::string&>& str_matcher)
      : str_matcher_(str_matcher) {}
  bool MatchAndExplain(
      turbo::string_view actual,
      MatchResultListener* listener) const override {
    return str_matcher_.MatchAndExplain(std::string(actual), listener);
  }
  void DescribeTo(std::ostream* os) const override {
    return str_matcher_.DescribeTo(os);
  }

  void DescribeNegationTo(std::ostream* os) const override {
    return str_matcher_.DescribeNegationTo(os);
  }

 private:
  const Matcher<const std::string&> str_matcher_;
};

class MatchesOstreamImpl final
    : public MatcherInterface<turbo::string_view> {
 public:
  explicit MatchesOstreamImpl(std::string expected)
      : expected_(std::move(expected)) {}
  bool MatchAndExplain(turbo::string_view actual,
                       MatchResultListener*) const override {
    return actual == expected_;
  }
  void DescribeTo(std::ostream* os) const override {
    *os << "matches the contents of the ostringstream, which are \""
        << expected_ << "\"";
  }

  void DescribeNegationTo(std::ostream* os) const override {
    *os << "does not match the contents of the ostringstream, which are \""
        << expected_ << "\"";
  }

 private:
  const std::string expected_;
};
}  // namespace

Matcher<turbo::string_view> AsString(
    const Matcher<const std::string&>& str_matcher) {
  return MakeMatcher(new AsStringImpl(str_matcher));
}

Matcher<const turbo::LogEntry&> SourceFilename(
    const Matcher<turbo::string_view>& source_filename) {
  return Property("source_filename", &turbo::LogEntry::source_filename,
                  source_filename);
}

Matcher<const turbo::LogEntry&> SourceBasename(
    const Matcher<turbo::string_view>& source_basename) {
  return Property("source_basename", &turbo::LogEntry::source_basename,
                  source_basename);
}

Matcher<const turbo::LogEntry&> SourceLine(
    const Matcher<int>& source_line) {
  return Property("source_line", &turbo::LogEntry::source_line, source_line);
}

Matcher<const turbo::LogEntry&> Prefix(
    const Matcher<bool>& prefix) {
  return Property("prefix", &turbo::LogEntry::prefix, prefix);
}

Matcher<const turbo::LogEntry&> LogSeverity(
    const Matcher<turbo::LogSeverity>& log_severity) {
  return Property("log_severity", &turbo::LogEntry::log_severity, log_severity);
}

Matcher<const turbo::LogEntry&> Timestamp(
    const Matcher<turbo::Time>& timestamp) {
  return Property("timestamp", &turbo::LogEntry::timestamp, timestamp);
}

Matcher<const turbo::LogEntry&> TimestampInMatchWindow() {
  return Property("timestamp", &turbo::LogEntry::timestamp,
                  AllOf(Ge(turbo::Time::current_time()), Truly([](turbo::Time arg) {
                          return arg <= turbo::Time::current_time();
                        })));
}

Matcher<const turbo::LogEntry&> ThreadID(
    const Matcher<turbo::LogEntry::tid_t>& tid) {
  return Property("tid", &turbo::LogEntry::tid, tid);
}

Matcher<const turbo::LogEntry&> TextMessageWithPrefixAndNewline(
    const Matcher<turbo::string_view>&
        text_message_with_prefix_and_newline) {
  return Property("text_message_with_prefix_and_newline",
                  &turbo::LogEntry::text_message_with_prefix_and_newline,
                  text_message_with_prefix_and_newline);
}

Matcher<const turbo::LogEntry&> TextMessageWithPrefix(
    const Matcher<turbo::string_view>& text_message_with_prefix) {
  return Property("text_message_with_prefix",
                  &turbo::LogEntry::text_message_with_prefix,
                  text_message_with_prefix);
}

Matcher<const turbo::LogEntry&> TextMessage(
    const Matcher<turbo::string_view>& text_message) {
  return Property("text_message", &turbo::LogEntry::text_message, text_message);
}

Matcher<const turbo::LogEntry&> TextPrefix(
    const Matcher<turbo::string_view>& text_prefix) {
  return ResultOf(
      [](const turbo::LogEntry& entry) {
        turbo::string_view msg = entry.text_message_with_prefix();
        msg.remove_suffix(entry.text_message().size());
        return msg;
      },
      text_prefix);
}
Matcher<const turbo::LogEntry&> RawEncodedMessage(
    const Matcher<turbo::string_view>& raw_encoded_message) {
  return Property("encoded_message", &turbo::LogEntry::encoded_message,
                  raw_encoded_message);
}

Matcher<const turbo::LogEntry&> Verbosity(
    const Matcher<int>& verbosity) {
  return Property("verbosity", &turbo::LogEntry::verbosity, verbosity);
}

Matcher<const turbo::LogEntry&> Stacktrace(
    const Matcher<turbo::string_view>& stacktrace) {
  return Property("stacktrace", &turbo::LogEntry::stacktrace, stacktrace);
}

Matcher<turbo::string_view> MatchesOstream(
    const std::ostringstream& stream) {
  return MakeMatcher(new MatchesOstreamImpl(stream.str()));
}

// We need to validate what is and isn't logged as the process dies due to
// `FATAL`, `QFATAL`, `CHECK`, etc., but assertions inside a death test
// subprocess don't directly affect the pass/fail status of the parent process.
// Instead, we use the mock actions `DeathTestExpectedLogging` and
// `DeathTestUnexpectedLogging` to write specific phrases to `stderr` that we
// can validate in the parent process using this matcher.
Matcher<const std::string&> DeathTestValidateExpectations() {
  if (log_internal::LoggingEnabledAt(turbo::LogSeverity::kFatal)) {
    return Matcher<const std::string&>(
        AllOf(HasSubstr("Mock received expected entry"),
              Not(HasSubstr("Mock received unexpected entry"))));
  }
  // If `FATAL` logging is disabled, neither message should have been written.
  return Matcher<const std::string&>(
      AllOf(Not(HasSubstr("Mock received expected entry")),
            Not(HasSubstr("Mock received unexpected entry"))));
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
