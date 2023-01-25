// Copyright 2018 The Abseil Authors.
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

#include "turbo/base/log_severity.h"

#include <cstdint>
#include <ios>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/flags/internal/flag.h"
#include "turbo/flags/marshalling.h"
#include "turbo/strings/str_cat.h"

namespace {
using ::testing::Eq;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::TestWithParam;
using ::testing::Values;

template <typename T>
std::string StreamHelper(T value) {
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

TEST(StreamTest, Works) {
  EXPECT_THAT(StreamHelper(static_cast<turbo::LogSeverity>(-100)),
              Eq("turbo::LogSeverity(-100)"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverity::kInfo), Eq("INFO"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverity::kWarning), Eq("WARNING"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverity::kError), Eq("ERROR"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverity::kFatal), Eq("FATAL"));
  EXPECT_THAT(StreamHelper(static_cast<turbo::LogSeverity>(4)),
              Eq("turbo::LogSeverity(4)"));
}

static_assert(turbo::flags_internal::FlagUseValueAndInitBitStorage<
                  turbo::LogSeverity>::value,
              "Flags of type turbo::LogSeverity ought to be lock-free.");

using ParseFlagFromOutOfRangeIntegerTest = TestWithParam<int64_t>;
INSTANTIATE_TEST_SUITE_P(
    Instantiation, ParseFlagFromOutOfRangeIntegerTest,
    Values(static_cast<int64_t>(std::numeric_limits<int>::min()) - 1,
           static_cast<int64_t>(std::numeric_limits<int>::max()) + 1));
TEST_P(ParseFlagFromOutOfRangeIntegerTest, ReturnsError) {
  const std::string to_parse = turbo::StrCat(GetParam());
  turbo::LogSeverity value;
  std::string error;
  EXPECT_THAT(turbo::ParseFlag(to_parse, &value, &error), IsFalse()) << value;
}

using ParseFlagFromAlmostOutOfRangeIntegerTest = TestWithParam<int>;
INSTANTIATE_TEST_SUITE_P(Instantiation,
                         ParseFlagFromAlmostOutOfRangeIntegerTest,
                         Values(std::numeric_limits<int>::min(),
                                std::numeric_limits<int>::max()));
TEST_P(ParseFlagFromAlmostOutOfRangeIntegerTest, YieldsExpectedValue) {
  const auto expected = static_cast<turbo::LogSeverity>(GetParam());
  const std::string to_parse = turbo::StrCat(GetParam());
  turbo::LogSeverity value;
  std::string error;
  ASSERT_THAT(turbo::ParseFlag(to_parse, &value, &error), IsTrue()) << error;
  EXPECT_THAT(value, Eq(expected));
}

using ParseFlagFromIntegerMatchingEnumeratorTest =
    TestWithParam<std::tuple<turbo::string_view, turbo::LogSeverity>>;
INSTANTIATE_TEST_SUITE_P(
    Instantiation, ParseFlagFromIntegerMatchingEnumeratorTest,
    Values(std::make_tuple("0", turbo::LogSeverity::kInfo),
           std::make_tuple(" 0", turbo::LogSeverity::kInfo),
           std::make_tuple("-0", turbo::LogSeverity::kInfo),
           std::make_tuple("+0", turbo::LogSeverity::kInfo),
           std::make_tuple("00", turbo::LogSeverity::kInfo),
           std::make_tuple("0 ", turbo::LogSeverity::kInfo),
           std::make_tuple("0x0", turbo::LogSeverity::kInfo),
           std::make_tuple("1", turbo::LogSeverity::kWarning),
           std::make_tuple("+1", turbo::LogSeverity::kWarning),
           std::make_tuple("2", turbo::LogSeverity::kError),
           std::make_tuple("3", turbo::LogSeverity::kFatal)));
TEST_P(ParseFlagFromIntegerMatchingEnumeratorTest, YieldsExpectedValue) {
  const turbo::string_view to_parse = std::get<0>(GetParam());
  const turbo::LogSeverity expected = std::get<1>(GetParam());
  turbo::LogSeverity value;
  std::string error;
  ASSERT_THAT(turbo::ParseFlag(to_parse, &value, &error), IsTrue()) << error;
  EXPECT_THAT(value, Eq(expected));
}

using ParseFlagFromOtherIntegerTest =
    TestWithParam<std::tuple<turbo::string_view, int>>;
INSTANTIATE_TEST_SUITE_P(Instantiation, ParseFlagFromOtherIntegerTest,
                         Values(std::make_tuple("-1", -1),
                                std::make_tuple("4", 4),
                                std::make_tuple("010", 10),
                                std::make_tuple("0x10", 16)));
TEST_P(ParseFlagFromOtherIntegerTest, YieldsExpectedValue) {
  const turbo::string_view to_parse = std::get<0>(GetParam());
  const auto expected = static_cast<turbo::LogSeverity>(std::get<1>(GetParam()));
  turbo::LogSeverity value;
  std::string error;
  ASSERT_THAT(turbo::ParseFlag(to_parse, &value, &error), IsTrue()) << error;
  EXPECT_THAT(value, Eq(expected));
}

using ParseFlagFromEnumeratorTest =
    TestWithParam<std::tuple<turbo::string_view, turbo::LogSeverity>>;
INSTANTIATE_TEST_SUITE_P(
    Instantiation, ParseFlagFromEnumeratorTest,
    Values(std::make_tuple("INFO", turbo::LogSeverity::kInfo),
           std::make_tuple("info", turbo::LogSeverity::kInfo),
           std::make_tuple("kInfo", turbo::LogSeverity::kInfo),
           std::make_tuple("iNfO", turbo::LogSeverity::kInfo),
           std::make_tuple("kInFo", turbo::LogSeverity::kInfo),
           std::make_tuple("WARNING", turbo::LogSeverity::kWarning),
           std::make_tuple("warning", turbo::LogSeverity::kWarning),
           std::make_tuple("kWarning", turbo::LogSeverity::kWarning),
           std::make_tuple("WaRnInG", turbo::LogSeverity::kWarning),
           std::make_tuple("KwArNiNg", turbo::LogSeverity::kWarning),
           std::make_tuple("ERROR", turbo::LogSeverity::kError),
           std::make_tuple("error", turbo::LogSeverity::kError),
           std::make_tuple("kError", turbo::LogSeverity::kError),
           std::make_tuple("eRrOr", turbo::LogSeverity::kError),
           std::make_tuple("kErRoR", turbo::LogSeverity::kError),
           std::make_tuple("FATAL", turbo::LogSeverity::kFatal),
           std::make_tuple("fatal", turbo::LogSeverity::kFatal),
           std::make_tuple("kFatal", turbo::LogSeverity::kFatal),
           std::make_tuple("FaTaL", turbo::LogSeverity::kFatal),
           std::make_tuple("KfAtAl", turbo::LogSeverity::kFatal)));
TEST_P(ParseFlagFromEnumeratorTest, YieldsExpectedValue) {
  const turbo::string_view to_parse = std::get<0>(GetParam());
  const turbo::LogSeverity expected = std::get<1>(GetParam());
  turbo::LogSeverity value;
  std::string error;
  ASSERT_THAT(turbo::ParseFlag(to_parse, &value, &error), IsTrue()) << error;
  EXPECT_THAT(value, Eq(expected));
}

using ParseFlagFromGarbageTest = TestWithParam<turbo::string_view>;
INSTANTIATE_TEST_SUITE_P(Instantiation, ParseFlagFromGarbageTest,
                         Values("", "\0", " ", "garbage", "kkinfo", "I"));
TEST_P(ParseFlagFromGarbageTest, ReturnsError) {
  const turbo::string_view to_parse = GetParam();
  turbo::LogSeverity value;
  std::string error;
  EXPECT_THAT(turbo::ParseFlag(to_parse, &value, &error), IsFalse()) << value;
}

using UnparseFlagToEnumeratorTest =
    TestWithParam<std::tuple<turbo::LogSeverity, turbo::string_view>>;
INSTANTIATE_TEST_SUITE_P(
    Instantiation, UnparseFlagToEnumeratorTest,
    Values(std::make_tuple(turbo::LogSeverity::kInfo, "INFO"),
           std::make_tuple(turbo::LogSeverity::kWarning, "WARNING"),
           std::make_tuple(turbo::LogSeverity::kError, "ERROR"),
           std::make_tuple(turbo::LogSeverity::kFatal, "FATAL")));
TEST_P(UnparseFlagToEnumeratorTest, ReturnsExpectedValueAndRoundTrips) {
  const turbo::LogSeverity to_unparse = std::get<0>(GetParam());
  const turbo::string_view expected = std::get<1>(GetParam());
  const std::string stringified_value = turbo::UnparseFlag(to_unparse);
  EXPECT_THAT(stringified_value, Eq(expected));
  turbo::LogSeverity reparsed_value;
  std::string error;
  EXPECT_THAT(turbo::ParseFlag(stringified_value, &reparsed_value, &error),
              IsTrue());
  EXPECT_THAT(reparsed_value, Eq(to_unparse));
}

using UnparseFlagToOtherIntegerTest = TestWithParam<int>;
INSTANTIATE_TEST_SUITE_P(Instantiation, UnparseFlagToOtherIntegerTest,
                         Values(std::numeric_limits<int>::min(), -1, 4,
                                std::numeric_limits<int>::max()));
TEST_P(UnparseFlagToOtherIntegerTest, ReturnsExpectedValueAndRoundTrips) {
  const turbo::LogSeverity to_unparse =
      static_cast<turbo::LogSeverity>(GetParam());
  const std::string expected = turbo::StrCat(GetParam());
  const std::string stringified_value = turbo::UnparseFlag(to_unparse);
  EXPECT_THAT(stringified_value, Eq(expected));
  turbo::LogSeverity reparsed_value;
  std::string error;
  EXPECT_THAT(turbo::ParseFlag(stringified_value, &reparsed_value, &error),
              IsTrue());
  EXPECT_THAT(reparsed_value, Eq(to_unparse));
}

TEST(LogThresholdTest, LogSeverityAtLeastTest) {
  EXPECT_LT(turbo::LogSeverity::kError, turbo::LogSeverityAtLeast::kFatal);
  EXPECT_GT(turbo::LogSeverityAtLeast::kError, turbo::LogSeverity::kInfo);

  EXPECT_LE(turbo::LogSeverityAtLeast::kInfo, turbo::LogSeverity::kError);
  EXPECT_GE(turbo::LogSeverity::kError, turbo::LogSeverityAtLeast::kInfo);
}

TEST(LogThresholdTest, LogSeverityAtMostTest) {
  EXPECT_GT(turbo::LogSeverity::kError, turbo::LogSeverityAtMost::kWarning);
  EXPECT_LT(turbo::LogSeverityAtMost::kError, turbo::LogSeverity::kFatal);

  EXPECT_GE(turbo::LogSeverityAtMost::kFatal, turbo::LogSeverity::kError);
  EXPECT_LE(turbo::LogSeverity::kWarning, turbo::LogSeverityAtMost::kError);
}

TEST(LogThresholdTest, Extremes) {
  EXPECT_LT(turbo::LogSeverity::kFatal, turbo::LogSeverityAtLeast::kInfinity);
  EXPECT_GT(turbo::LogSeverity::kInfo,
            turbo::LogSeverityAtMost::kNegativeInfinity);
}

TEST(LogThresholdTest, Output) {
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtLeast::kInfo), Eq(">=INFO"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtLeast::kWarning),
              Eq(">=WARNING"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtLeast::kError), Eq(">=ERROR"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtLeast::kFatal), Eq(">=FATAL"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtLeast::kInfinity),
              Eq("INFINITY"));

  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtMost::kInfo), Eq("<=INFO"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtMost::kWarning), Eq("<=WARNING"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtMost::kError), Eq("<=ERROR"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtMost::kFatal), Eq("<=FATAL"));
  EXPECT_THAT(StreamHelper(turbo::LogSeverityAtMost::kNegativeInfinity),
              Eq("NEGATIVE_INFINITY"));
}

}  // namespace
