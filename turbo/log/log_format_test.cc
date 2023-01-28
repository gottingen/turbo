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

#include <math.h>

#include <optional>
#include <iomanip>
#include <ios>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

#ifdef __ANDROID__
#include <android/api-level.h>
#endif
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/log/check.h"
#include "turbo/log/internal/test_matchers.h"
#include "turbo/log/log.h"
#include "turbo/log/scoped_mock_log.h"
#include "turbo/strings/match.h"
#include "turbo/strings/str_cat.h"
#include "turbo/strings/str_format.h"
#include "turbo/strings/string_view.h"

namespace {
using ::turbo::log_internal::AsString;
using ::turbo::log_internal::MatchesOstream;
using ::turbo::log_internal::RawEncodedMessage;
using ::turbo::log_internal::TextMessage;
using ::turbo::log_internal::TextPrefix;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Each;
using ::testing::EndsWith;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::SizeIs;
using ::testing::Types;

// Some aspects of formatting streamed data (e.g. pointer handling) are
// implementation-defined.  Others are buggy in supported implementations.
// These tests validate that the formatting matches that performed by a
// `std::ostream` and also that the result is one of a list of expected formats.

std::ostringstream ComparisonStream() {
  std::ostringstream str;
  str.setf(std::ios_base::showbase | std::ios_base::boolalpha |
           std::ios_base::internal);
  return str;
}

TEST(LogFormatTest, NoMessage) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO); };

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(ComparisonStream())),
                         TextPrefix(AsString(EndsWith(turbo::StrCat(
                             " log_format_test.cc:", log_line, "] ")))),
                         TextMessage(IsEmpty()),
                         ENCODED_MESSAGE(EqualsProto(R"pb()pb")))));

  test_sink.StartCapturingLogs();
  do_log();
}

template <typename T>
class CharLogFormatTest : public testing::Test {};
using CharTypes = Types<char, signed char, unsigned char>;
TYPED_TEST_SUITE(CharLogFormatTest, CharTypes);

TYPED_TEST(CharLogFormatTest, Printable) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = 'x';
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("x")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "x" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(CharLogFormatTest, Unprintable) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  constexpr auto value = static_cast<TypeParam>(0xeeu);
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("\xee")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "\xee"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

template <typename T>
class UnsignedIntLogFormatTest : public testing::Test {};
using UnsignedIntTypes = Types<unsigned short, unsigned int,        // NOLINT
                               unsigned long, unsigned long long>;  // NOLINT
TYPED_TEST_SUITE(UnsignedIntLogFormatTest, UnsignedIntTypes);

TYPED_TEST(UnsignedIntLogFormatTest, Positive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = 224;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("224")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "224" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(UnsignedIntLogFormatTest, BitfieldPositive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const struct {
    TypeParam bits : 6;
  } value{42};
  auto comparison_stream = ComparisonStream();
  comparison_stream << value.bits;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("42")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "42" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value.bits;
}

template <typename T>
class SignedIntLogFormatTest : public testing::Test {};
using SignedIntTypes =
    Types<signed short, signed int, signed long, signed long long>;  // NOLINT
TYPED_TEST_SUITE(SignedIntLogFormatTest, SignedIntTypes);

TYPED_TEST(SignedIntLogFormatTest, Positive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = 224;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("224")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "224" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(SignedIntLogFormatTest, Negative) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = -112;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("-112")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "-112"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(SignedIntLogFormatTest, BitfieldPositive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const struct {
    TypeParam bits : 6;
  } value{21};
  auto comparison_stream = ComparisonStream();
  comparison_stream << value.bits;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("21")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "21" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value.bits;
}

TYPED_TEST(SignedIntLogFormatTest, BitfieldNegative) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const struct {
    TypeParam bits : 6;
  } value{-21};
  auto comparison_stream = ComparisonStream();
  comparison_stream << value.bits;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("-21")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "-21" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value.bits;
}

// Ignore these test cases on GCC due to "is too small to hold all values ..."
// warning.
#if !defined(__GNUC__) || defined(__clang__)
// The implementation may choose a signed or unsigned integer type to represent
// this enum, so it may be tested by either `UnsignedEnumLogFormatTest` or
// `SignedEnumLogFormatTest`.
enum MyUnsignedEnum {
  MyUnsignedEnum_ZERO = 0,
  MyUnsignedEnum_FORTY_TWO = 42,
  MyUnsignedEnum_TWO_HUNDRED_TWENTY_FOUR = 224,
};
enum MyUnsignedIntEnum : unsigned int {
  MyUnsignedIntEnum_ZERO = 0,
  MyUnsignedIntEnum_FORTY_TWO = 42,
  MyUnsignedIntEnum_TWO_HUNDRED_TWENTY_FOUR = 224,
};

template <typename T>
class UnsignedEnumLogFormatTest : public testing::Test {};
using UnsignedEnumTypes = std::conditional<
    std::is_signed<std::underlying_type<MyUnsignedEnum>::type>::value,
    Types<MyUnsignedIntEnum>, Types<MyUnsignedEnum, MyUnsignedIntEnum>>::type;
TYPED_TEST_SUITE(UnsignedEnumLogFormatTest, UnsignedEnumTypes);

TYPED_TEST(UnsignedEnumLogFormatTest, Positive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = static_cast<TypeParam>(224);
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("224")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "224" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(UnsignedEnumLogFormatTest, BitfieldPositive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const struct {
    TypeParam bits : 6;
  } value{static_cast<TypeParam>(42)};
  auto comparison_stream = ComparisonStream();
  comparison_stream << value.bits;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("42")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "42" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value.bits;
}

enum MySignedEnum {
  MySignedEnum_NEGATIVE_ONE_HUNDRED_TWELVE = -112,
  MySignedEnum_NEGATIVE_TWENTY_ONE = -21,
  MySignedEnum_ZERO = 0,
  MySignedEnum_TWENTY_ONE = 21,
  MySignedEnum_TWO_HUNDRED_TWENTY_FOUR = 224,
};
enum MySignedIntEnum : signed int {
  MySignedIntEnum_NEGATIVE_ONE_HUNDRED_TWELVE = -112,
  MySignedIntEnum_NEGATIVE_TWENTY_ONE = -21,
  MySignedIntEnum_ZERO = 0,
  MySignedIntEnum_TWENTY_ONE = 21,
  MySignedIntEnum_TWO_HUNDRED_TWENTY_FOUR = 224,
};

template <typename T>
class SignedEnumLogFormatTest : public testing::Test {};
using SignedEnumTypes = std::conditional<
    std::is_signed<std::underlying_type<MyUnsignedEnum>::type>::value,
    Types<MyUnsignedEnum, MySignedEnum, MySignedIntEnum>,
    Types<MySignedEnum, MySignedIntEnum>>::type;
TYPED_TEST_SUITE(SignedEnumLogFormatTest, SignedEnumTypes);

TYPED_TEST(SignedEnumLogFormatTest, Positive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = static_cast<TypeParam>(224);
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("224")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "224" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(SignedEnumLogFormatTest, Negative) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = static_cast<TypeParam>(-112);
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("-112")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "-112"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(SignedEnumLogFormatTest, BitfieldPositive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const struct {
    TypeParam bits : 6;
  } value{static_cast<TypeParam>(21)};
  auto comparison_stream = ComparisonStream();
  comparison_stream << value.bits;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("21")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "21" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value.bits;
}

TYPED_TEST(SignedEnumLogFormatTest, BitfieldNegative) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const struct {
    TypeParam bits : 6;
  } value{static_cast<TypeParam>(-21)};
  auto comparison_stream = ComparisonStream();
  comparison_stream << value.bits;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("-21")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "-21" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value.bits;
}
#endif

TEST(FloatLogFormatTest, Positive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const float value = 6.02e23f;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("6.02e+23")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "6.02e+23"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TEST(FloatLogFormatTest, Negative) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const float value = -6.02e23f;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("-6.02e+23")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "-6.02e+23"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TEST(FloatLogFormatTest, NegativeExponent) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const float value = 6.02e-23f;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("6.02e-23")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "6.02e-23"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TEST(DoubleLogFormatTest, Positive) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 6.02e23;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("6.02e+23")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "6.02e+23"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TEST(DoubleLogFormatTest, Negative) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = -6.02e23;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("-6.02e+23")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "-6.02e+23"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TEST(DoubleLogFormatTest, NegativeExponent) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 6.02e-23;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("6.02e-23")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "6.02e-23"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

template <typename T>
class FloatingPointLogFormatTest : public testing::Test {};
using FloatingPointTypes = Types<float, double>;
TYPED_TEST_SUITE(FloatingPointLogFormatTest, FloatingPointTypes);

TYPED_TEST(FloatingPointLogFormatTest, Zero) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = 0.0;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("0")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "0" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(FloatingPointLogFormatTest, Integer) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = 1.0;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("1")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "1" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(FloatingPointLogFormatTest, Infinity) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = std::numeric_limits<TypeParam>::infinity();
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(AnyOf(Eq("inf"), Eq("Inf"))),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "inf" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(FloatingPointLogFormatTest, NegativeInfinity) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = -std::numeric_limits<TypeParam>::infinity();
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(AnyOf(Eq("-inf"), Eq("-Inf"))),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "-inf"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(FloatingPointLogFormatTest, NaN) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = std::numeric_limits<TypeParam>::quiet_NaN();
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(AnyOf(Eq("nan"), Eq("NaN"))),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "nan" })pb")))));
  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(FloatingPointLogFormatTest, NegativeNaN) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value =
      std::copysign(std::numeric_limits<TypeParam>::quiet_NaN(), -1.0);
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(MatchesOstream(comparison_stream)),
          TextMessage(AnyOf(Eq("-nan"), Eq("nan"), Eq("NaN"), Eq("-nan(ind)"))),
          ENCODED_MESSAGE(
              AnyOf(EqualsProto(R"pb(value { str: "-nan" })pb"),
                    EqualsProto(R"pb(value { str: "nan" })pb"),
                    EqualsProto(R"pb(value { str: "-nan(ind)" })pb"))))));
  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

template <typename T>
class VoidPtrLogFormatTest : public testing::Test {};
using VoidPtrTypes = Types<void *, const void *>;
TYPED_TEST_SUITE(VoidPtrLogFormatTest, VoidPtrTypes);

TYPED_TEST(VoidPtrLogFormatTest, Null) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = nullptr;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(AnyOf(Eq("(nil)"), Eq("0"), Eq("0x0"),
                                   Eq("00000000"), Eq("0000000000000000"))))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(VoidPtrLogFormatTest, NonNull) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = reinterpret_cast<TypeParam>(0xdeadbeefULL);
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(MatchesOstream(comparison_stream)),
          TextMessage(
              AnyOf(Eq("0xdeadbeef"), Eq("DEADBEEF"), Eq("00000000DEADBEEF"))),
          ENCODED_MESSAGE(AnyOf(
              EqualsProto(R"pb(value { str: "0xdeadbeef" })pb"),
              EqualsProto(R"pb(value { str: "00000000DEADBEEF" })pb"))))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

template <typename T>
class VolatilePtrLogFormatTest : public testing::Test {};
using VolatilePtrTypes =
    Types<volatile void*, const volatile void*, volatile char*,
          const volatile char*, volatile signed char*,
          const volatile signed char*, volatile unsigned char*,
          const volatile unsigned char*>;
TYPED_TEST_SUITE(VolatilePtrLogFormatTest, VolatilePtrTypes);

TYPED_TEST(VolatilePtrLogFormatTest, Null) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = nullptr;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("false")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "false"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(VolatilePtrLogFormatTest, NonNull) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const TypeParam value = reinterpret_cast<TypeParam>(0xdeadbeefLL);
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("true")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "true"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

template <typename T>
class CharPtrLogFormatTest : public testing::Test {};
using CharPtrTypes = Types<char, const char, signed char, const signed char,
                           unsigned char, const unsigned char>;
TYPED_TEST_SUITE(CharPtrLogFormatTest, CharPtrTypes);

TYPED_TEST(CharPtrLogFormatTest, Null) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  // Streaming `([cv] char *)nullptr` into a `std::ostream` is UB, and some C++
  // standard library implementations choose to crash.  We take measures to log
  // something useful instead of crashing, even when that differs from the
  // standard library in use (and thus the behavior of `std::ostream`).
  TypeParam* const value = nullptr;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          // `MatchesOstream` deliberately omitted since we deliberately differ.
          TextMessage(Eq("(null)")),
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "(null)" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TYPED_TEST(CharPtrLogFormatTest, NonNull) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  TypeParam data[] = {'v', 'a', 'l', 'u', 'e', '\0'};
  TypeParam* const value = data;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("value")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "value"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TEST(BoolLogFormatTest, True) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const bool value = true;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("true")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "true"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TEST(BoolLogFormatTest, False) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const bool value = false;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("false")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "false"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

TEST(LogFormatTest, StringLiteral) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  auto comparison_stream = ComparisonStream();
  comparison_stream << "value";

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("value")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            literal: "value"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << "value";
}

TEST(LogFormatTest, CharArray) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  char value[] = "value";
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("value")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "value"
                                                             })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

class CustomClass {};
std::ostream& operator<<(std::ostream& os, const CustomClass&) {
  return os << "CustomClass{}";
}

TEST(LogFormatTest, Custom) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  CustomClass value;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("CustomClass{}")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "CustomClass{}"
                                                          })pb")))));
  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

class CustomClassNonCopyable {
 public:
  CustomClassNonCopyable() = default;
  CustomClassNonCopyable(const CustomClassNonCopyable&) = delete;
  CustomClassNonCopyable& operator=(const CustomClassNonCopyable&) = delete;
};
std::ostream& operator<<(std::ostream& os, const CustomClassNonCopyable&) {
  return os << "CustomClassNonCopyable{}";
}

TEST(LogFormatTest, CustomNonCopyable) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  CustomClassNonCopyable value;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("CustomClassNonCopyable{}")),
                 ENCODED_MESSAGE(EqualsProto(
                     R"pb(value { str: "CustomClassNonCopyable{}" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value;
}

struct Point {
  template <typename Sink>
  friend void TurboStringify(Sink& sink, const Point& p) {
    turbo::Format(&sink, "(%d, %d)", p.x, p.y);
  }

  int x = 10;
  int y = 20;
};

TEST(LogFormatTest, TurboStringifyExample) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  Point p;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(Eq("(10, 20)")), TextMessage(Eq(turbo::StrCat(p))),
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "(10, 20)" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << p;
}

struct PointWithTurboStringifiyAndOstream {
  template <typename Sink>
  friend void TurboStringify(Sink& sink,
                            const PointWithTurboStringifiyAndOstream& p) {
    turbo::Format(&sink, "(%d, %d)", p.x, p.y);
  }

  int x = 10;
  int y = 20;
};

TURBO_ATTRIBUTE_UNUSED std::ostream& operator<<(
    std::ostream& os, const PointWithTurboStringifiyAndOstream&) {
  return os << "Default to TurboStringify()";
}

TEST(LogFormatTest, CustomWithTurboStringifyAndOstream) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  PointWithTurboStringifiyAndOstream p;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(Eq("(10, 20)")), TextMessage(Eq(turbo::StrCat(p))),
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "(10, 20)" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << p;
}

struct PointStreamsNothing {
  template <typename Sink>
  friend void TurboStringify(Sink&, const PointStreamsNothing&) {}

  int x = 10;
  int y = 20;
};

TEST(LogFormatTest, TurboStringifyStreamsNothing) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  PointStreamsNothing p;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(Eq("77")), TextMessage(Eq(turbo::StrCat(p, 77))),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << p << 77;
}

struct PointMultipleAppend {
  template <typename Sink>
  friend void TurboStringify(Sink& sink, const PointMultipleAppend& p) {
    sink.Append("(");
    sink.Append(turbo::StrCat(p.x, ", ", p.y, ")"));
  }

  int x = 10;
  int y = 20;
};

TEST(LogFormatTest, TurboStringifyMultipleAppend) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  PointMultipleAppend p;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(Eq("(10, 20)")), TextMessage(Eq(turbo::StrCat(p))),
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "(" }
                                           value { str: "10, 20)" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << p;
}

TEST(ManipulatorLogFormatTest, BoolAlphaTrue) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const bool value = true;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::noboolalpha << value << " "  //
                    << std::boolalpha << value << " "    //
                    << std::noboolalpha << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("1 true 1")),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { str: "1" }
                                  value { literal: " " }
                                  value { str: "true" }
                                  value { literal: " " }
                                  value { str: "1" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::noboolalpha << value << " "  //
            << std::boolalpha << value << " "    //
            << std::noboolalpha << value;
}

TEST(ManipulatorLogFormatTest, BoolAlphaFalse) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const bool value = false;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::noboolalpha << value << " "  //
                    << std::boolalpha << value << " "    //
                    << std::noboolalpha << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("0 false 0")),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { str: "0" }
                                  value { literal: " " }
                                  value { str: "false" }
                                  value { literal: " " }
                                  value { str: "0" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::noboolalpha << value << " "  //
            << std::boolalpha << value << " "    //
            << std::noboolalpha << value;
}

TEST(ManipulatorLogFormatTest, ShowPoint) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 77.0;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::noshowpoint << value << " "  //
                    << std::showpoint << value << " "    //
                    << std::noshowpoint << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("77 77.0000 77")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "77" }
                                                  value { literal: " " }
                                                  value { str: "77.0000" }
                                                  value { literal: " " }
                                                  value { str: "77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::noshowpoint << value << " "  //
            << std::showpoint << value << " "    //
            << std::noshowpoint << value;
}

TEST(ManipulatorLogFormatTest, ShowPos) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::noshowpos << value << " "  //
                    << std::showpos << value << " "    //
                    << std::noshowpos << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("77 +77 77")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "77" }
                                                  value { literal: " " }
                                                  value { str: "+77" }
                                                  value { literal: " " }
                                                  value { str: "77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::noshowpos << value << " "  //
            << std::showpos << value << " "    //
            << std::noshowpos << value;
}

TEST(ManipulatorLogFormatTest, UppercaseFloat) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 7.7e7;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::nouppercase << value << " "  //
                    << std::uppercase << value << " "    //
                    << std::nouppercase << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("7.7e+07 7.7E+07 7.7e+07")),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { str: "7.7e+07" }
                                  value { literal: " " }
                                  value { str: "7.7E+07" }
                                  value { literal: " " }
                                  value { str: "7.7e+07" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::nouppercase << value << " "  //
            << std::uppercase << value << " "    //
            << std::nouppercase << value;
}

TEST(ManipulatorLogFormatTest, Hex) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 0x77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::hex << value;

  EXPECT_CALL(
      test_sink, Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                            TextMessage(Eq("0x77")),
                            ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                               str: "0x77"
                                                             })pb")))));
  test_sink.StartCapturingLogs();
  LOG(INFO) << std::hex << value;
}

TEST(ManipulatorLogFormatTest, Oct) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 077;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::oct << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("077")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "077" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::oct << value;
}

TEST(ManipulatorLogFormatTest, Dec) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::hex << std::dec << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("77")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::hex << std::dec << value;
}

TEST(ManipulatorLogFormatTest, ShowbaseHex) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 0x77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::hex                         //
                    << std::noshowbase << value << " "  //
                    << std::showbase << value << " "    //
                    << std::noshowbase << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("77 0x77 77")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "77" }
                                                  value { literal: " " }
                                                  value { str: "0x77" }
                                                  value { literal: " " }
                                                  value { str: "77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::hex                         //
            << std::noshowbase << value << " "  //
            << std::showbase << value << " "    //
            << std::noshowbase << value;
}

TEST(ManipulatorLogFormatTest, ShowbaseOct) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 077;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::oct                         //
                    << std::noshowbase << value << " "  //
                    << std::showbase << value << " "    //
                    << std::noshowbase << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("77 077 77")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "77" }
                                                  value { literal: " " }
                                                  value { str: "077" }
                                                  value { literal: " " }
                                                  value { str: "77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::oct                         //
            << std::noshowbase << value << " "  //
            << std::showbase << value << " "    //
            << std::noshowbase << value;
}

TEST(ManipulatorLogFormatTest, UppercaseHex) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 0xbeef;
  auto comparison_stream = ComparisonStream();
  comparison_stream                        //
      << std::hex                          //
      << std::nouppercase << value << " "  //
      << std::uppercase << value << " "    //
      << std::nouppercase << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("0xbeef 0XBEEF 0xbeef")),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { str: "0xbeef" }
                                  value { literal: " " }
                                  value { str: "0XBEEF" }
                                  value { literal: " " }
                                  value { str: "0xbeef" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::hex                          //
            << std::nouppercase << value << " "  //
            << std::uppercase << value << " "    //
            << std::nouppercase << value;
}

TEST(ManipulatorLogFormatTest, FixedFloat) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 7.7e7;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::fixed << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("77000000.000000")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                    str: "77000000.000000"
                                                  })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::fixed << value;
}

TEST(ManipulatorLogFormatTest, ScientificFloat) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 7.7e7;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::scientific << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("7.700000e+07")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "7.700000e+07"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::scientific << value;
}

#if defined(__BIONIC__) && (!defined(__ANDROID_API__) || __ANDROID_API__ < 22)
// Bionic doesn't support `%a` until API 22, so this prints 'a' even if the
// C++ standard library implements it correctly (by forwarding to printf).
#else
TEST(ManipulatorLogFormatTest, FixedAndScientificFloat) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 7.7e7;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::setiosflags(std::ios_base::scientific |
                                        std::ios_base::fixed)
                    << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(AnyOf(Eq("0x1.25bb50p+26"), Eq("0x1.25bb5p+26"),
                                   Eq("0x1.25bb500000000p+26"))),
                 ENCODED_MESSAGE(
                     AnyOf(EqualsProto(R"pb(value { str: "0x1.25bb5p+26" })pb"),
                           EqualsProto(R"pb(value {
                                              str: "0x1.25bb500000000p+26"
                                            })pb"))))));

  test_sink.StartCapturingLogs();

  // This combination should mean the same thing as `std::hexfloat`.
  LOG(INFO) << std::setiosflags(std::ios_base::scientific |
                                std::ios_base::fixed)
            << value;
}
#endif

#if defined(__BIONIC__) && (!defined(__ANDROID_API__) || __ANDROID_API__ < 22)
// Bionic doesn't support `%a` until API 22, so this prints 'a' even if the C++
// standard library supports `std::hexfloat` (by forwarding to printf).
#else
TEST(ManipulatorLogFormatTest, HexfloatFloat) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 7.7e7;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::hexfloat << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(AnyOf(Eq("0x1.25bb50p+26"), Eq("0x1.25bb5p+26"),
                                   Eq("0x1.25bb500000000p+26"))),
                 ENCODED_MESSAGE(
                     AnyOf(EqualsProto(R"pb(value { str: "0x1.25bb5p+26" })pb"),
                           EqualsProto(R"pb(value {
                                              str: "0x1.25bb500000000p+26"
                                            })pb"))))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::hexfloat << value;
}
#endif

TEST(ManipulatorLogFormatTest, DefaultFloatFloat) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 7.7e7;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::hexfloat << std::defaultfloat << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("7.7e+07")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "7.7e+07"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::hexfloat << std::defaultfloat << value;
}

TEST(ManipulatorLogFormatTest, Ends) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  auto comparison_stream = ComparisonStream();
  comparison_stream << std::ends;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq(std::string_view("\0", 1))),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "\0" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::ends;
}

TEST(ManipulatorLogFormatTest, Endl) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  auto comparison_stream = ComparisonStream();
  comparison_stream << std::endl;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(MatchesOstream(comparison_stream)),
          TextMessage(Eq("\n")),
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "\n" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::endl;
}

TEST(ManipulatorLogFormatTest, SetIosFlags) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 0x77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::resetiosflags(std::ios_base::basefield)
                    << std::setiosflags(std::ios_base::hex) << value << " "  //
                    << std::resetiosflags(std::ios_base::basefield)
                    << std::setiosflags(std::ios_base::dec) << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(MatchesOstream(comparison_stream)),
          TextMessage(Eq("0x77 119")),
          // `std::setiosflags` and `std::resetiosflags` aren't manipulators.
          // We're unable to distinguish their return type(s) from arbitrary
          // user-defined types and thus don't suppress the empty str value.
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "0x77" }
                                           value { literal: " " }
                                           value { str: "119" }
          )pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::resetiosflags(std::ios_base::basefield)
            << std::setiosflags(std::ios_base::hex) << value << " "  //
            << std::resetiosflags(std::ios_base::basefield)
            << std::setiosflags(std::ios_base::dec) << value;
}

TEST(ManipulatorLogFormatTest, SetBase) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 0x77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::setbase(16) << value << " "  //
                    << std::setbase(0) << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("0x77 119")),
                 // `std::setbase` isn't a manipulator.  We're unable to
                 // distinguish its return type from arbitrary user-defined
                 // types and thus don't suppress the empty str value.
                 ENCODED_MESSAGE(EqualsProto(
                     R"pb(value { str: "0x77" }
                          value { literal: " " }
                          value { str: "119" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::setbase(16) << value << " "  //
            << std::setbase(0) << value;
}

TEST(ManipulatorLogFormatTest, SetPrecision) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 6.022140857e23;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::setprecision(4) << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(MatchesOstream(comparison_stream)),
          TextMessage(Eq("6.022e+23")),
          // `std::setprecision` isn't a manipulator.  We're unable to
          // distinguish its return type from arbitrary user-defined
          // types and thus don't suppress the empty str value.
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "6.022e+23" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::setprecision(4) << value;
}

TEST(ManipulatorLogFormatTest, SetPrecisionOverflow) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const double value = 6.022140857e23;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::setprecision(200) << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("602214085700000015187968")),
                 ENCODED_MESSAGE(EqualsProto(
                     R"pb(value { str: "602214085700000015187968" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::setprecision(200) << value;
}

TEST(ManipulatorLogFormatTest, SetW) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::setw(8) << value;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(MatchesOstream(comparison_stream)),
          TextMessage(Eq("      77")),
          // `std::setw` isn't a manipulator.  We're unable to
          // distinguish its return type from arbitrary user-defined
          // types and thus don't suppress the empty str value.
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "      77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::setw(8) << value;
}

TEST(ManipulatorLogFormatTest, Left) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = -77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::left << std::setw(8) << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("-77     ")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "-77     "
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::left << std::setw(8) << value;
}

TEST(ManipulatorLogFormatTest, Right) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = -77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::right << std::setw(8) << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("     -77")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "     -77"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::right << std::setw(8) << value;
}

TEST(ManipulatorLogFormatTest, Internal) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = -77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::internal << std::setw(8) << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("-     77")),
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "-     77"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::internal << std::setw(8) << value;
}

TEST(ManipulatorLogFormatTest, SetFill) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int value = 77;
  auto comparison_stream = ComparisonStream();
  comparison_stream << std::setfill('0') << std::setw(8) << value;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("00000077")),
                         // `std::setfill` isn't a manipulator.  We're
                         // unable to distinguish its return
                         // type from arbitrary user-defined types and
                         // thus don't suppress the empty str value.
                         ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                            str: "00000077"
                                                          })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::setfill('0') << std::setw(8) << value;
}

class FromCustomClass {};
std::ostream& operator<<(std::ostream& os, const FromCustomClass&) {
  return os << "FromCustomClass{}" << std::hex;
}

TEST(ManipulatorLogFormatTest, FromCustom) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  FromCustomClass value;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value << " " << 0x77;

  EXPECT_CALL(test_sink,
              Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                         TextMessage(Eq("FromCustomClass{} 0x77")),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { str: "FromCustomClass{}" }
                                  value { literal: " " }
                                  value { str: "0x77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value << " " << 0x77;
}

class StreamsNothing {};
std::ostream& operator<<(std::ostream& os, const StreamsNothing&) { return os; }

TEST(ManipulatorLogFormatTest, CustomClassStreamsNothing) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  StreamsNothing value;
  auto comparison_stream = ComparisonStream();
  comparison_stream << value << 77;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(TextMessage(MatchesOstream(comparison_stream)),
                 TextMessage(Eq("77")),
                 ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "77" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << value << 77;
}

struct PointPercentV {
  template <typename Sink>
  friend void TurboStringify(Sink& sink, const PointPercentV& p) {
    turbo::Format(&sink, "(%v, %v)", p.x, p.y);
  }

  int x = 10;
  int y = 20;
};

TEST(ManipulatorLogFormatTest, IOManipsDoNotAffectTurboStringify) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  PointPercentV p;

  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(Eq("(10, 20)")), TextMessage(Eq(turbo::StrCat(p))),
          ENCODED_MESSAGE(EqualsProto(R"pb(value { str: "(10, 20)" })pb")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::hex << p;
}

TEST(StructuredLoggingOverflowTest, TruncatesStrings) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  // This message is too long and should be truncated to some unspecified size
  // no greater than the buffer size but not too much less either.  It should be
  // truncated rather than discarded.
  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(AllOf(
              SizeIs(AllOf(Ge(turbo::log_internal::kLogMessageBufferSize - 256),
                           Le(turbo::log_internal::kLogMessageBufferSize))),
              Each(Eq('x')))),
          ENCODED_MESSAGE(HasOneStrThat(AllOf(
              SizeIs(AllOf(Ge(turbo::log_internal::kLogMessageBufferSize - 256),
                           Le(turbo::log_internal::kLogMessageBufferSize))),
              Each(Eq('x'))))))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << std::string(2 * turbo::log_internal::kLogMessageBufferSize, 'x');
}

struct StringLike {
  std::string_view data;
};
std::ostream& operator<<(std::ostream& os, StringLike str) {
  return os << str.data;
}

TEST(StructuredLoggingOverflowTest, TruncatesInsertionOperators) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  // This message is too long and should be truncated to some unspecified size
  // no greater than the buffer size but not too much less either.  It should be
  // truncated rather than discarded.
  EXPECT_CALL(
      test_sink,
      Send(AllOf(
          TextMessage(AllOf(
              SizeIs(AllOf(Ge(turbo::log_internal::kLogMessageBufferSize - 256),
                           Le(turbo::log_internal::kLogMessageBufferSize))),
              Each(Eq('x')))),
          ENCODED_MESSAGE(HasOneStrThat(AllOf(
              SizeIs(AllOf(Ge(turbo::log_internal::kLogMessageBufferSize - 256),
                           Le(turbo::log_internal::kLogMessageBufferSize))),
              Each(Eq('x'))))))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << StringLike{
      std::string(2 * turbo::log_internal::kLogMessageBufferSize, 'x')};
}

// Returns the size of the largest string that will fit in a `LOG` message
// buffer with no prefix.
size_t MaxLogFieldLengthNoPrefix() {
  class StringLengthExtractorSink : public turbo::LogSink {
   public:
    void Send(const turbo::LogEntry& entry) override {
      CHECK(!size_.has_value());
      CHECK_EQ(entry.text_message().find_first_not_of('x'),
               std::string_view::npos);
      size_.emplace(entry.text_message().size());
    }
    size_t size() const {
      CHECK(size_.has_value());
      return *size_;
    }

   private:
    std::optional<size_t> size_;
  } extractor_sink;
  LOG(INFO).NoPrefix().ToSinkOnly(&extractor_sink)
      << std::string(2 * turbo::log_internal::kLogMessageBufferSize, 'x');
  return extractor_sink.size();
}

TEST(StructuredLoggingOverflowTest, TruncatesStringsCleanly) {
  const size_t longest_fit = MaxLogFieldLengthNoPrefix();
  // To log a second value field, we need four bytes: two tag/type bytes and two
  // sizes.  To put any data in the field we need a fifth byte.
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits exactly, no part of y fits.
    LOG(INFO).NoPrefix() << std::string(longest_fit, 'x') << "y";
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit - 1), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits, one byte from y's header fits but shouldn't be visible.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 1, 'x') << "y";
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit - 2), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits, two bytes from y's header fit but shouldn't be visible.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 2, 'x') << "y";
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit - 3), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits, three bytes from y's header fit but shouldn't be visible.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 3, 'x') << "y";
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrAndOneLiteralThat(
                               AllOf(SizeIs(longest_fit - 4), Each(Eq('x'))),
                               IsEmpty())),
                           RawEncodedMessage(Not(AsString(EndsWith("x")))))));
    test_sink.StartCapturingLogs();
    // x fits, all four bytes from y's header fit but no data bytes do, so we
    // encode an empty string.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 4, 'x') << "y";
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(
        test_sink,
        Send(AllOf(ENCODED_MESSAGE(HasOneStrAndOneLiteralThat(
                       AllOf(SizeIs(longest_fit - 5), Each(Eq('x'))), Eq("y"))),
                   RawEncodedMessage(AsString(EndsWith("y"))))));
    test_sink.StartCapturingLogs();
    // x fits, y fits exactly.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 5, 'x') << "y";
  }
}

TEST(StructuredLoggingOverflowTest, TruncatesInsertionOperatorsCleanly) {
  const size_t longest_fit = MaxLogFieldLengthNoPrefix();
  // To log a second value field, we need four bytes: two tag/type bytes and two
  // sizes.  To put any data in the field we need a fifth byte.
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits exactly, no part of y fits.
    LOG(INFO).NoPrefix() << std::string(longest_fit, 'x') << StringLike{"y"};
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit - 1), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits, one byte from y's header fits but shouldn't be visible.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 1, 'x')
                         << StringLike{"y"};
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit - 2), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits, two bytes from y's header fit but shouldn't be visible.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 2, 'x')
                         << StringLike{"y"};
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit - 3), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits, three bytes from y's header fit but shouldn't be visible.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 3, 'x')
                         << StringLike{"y"};
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(test_sink,
                Send(AllOf(ENCODED_MESSAGE(HasOneStrThat(
                               AllOf(SizeIs(longest_fit - 4), Each(Eq('x'))))),
                           RawEncodedMessage(AsString(EndsWith("x"))))));
    test_sink.StartCapturingLogs();
    // x fits, all four bytes from y's header fit but no data bytes do.  We
    // don't encode an empty string here because every I/O manipulator hits this
    // codepath and those shouldn't leave empty strings behind.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 4, 'x')
                         << StringLike{"y"};
  }
  {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(
        test_sink,
        Send(AllOf(ENCODED_MESSAGE(HasTwoStrsThat(
                       AllOf(SizeIs(longest_fit - 5), Each(Eq('x'))), Eq("y"))),
                   RawEncodedMessage(AsString(EndsWith("y"))))));
    test_sink.StartCapturingLogs();
    // x fits, y fits exactly.
    LOG(INFO).NoPrefix() << std::string(longest_fit - 5, 'x')
                         << StringLike{"y"};
  }
}

}  // namespace
