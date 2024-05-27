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

#include <turbo/flags/flag.h>

#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/macros.h>
#include <turbo/flags/config.h>
#include <turbo/flags/declare.h>
#include <turbo/flags/internal/flag.h>
#include <turbo/flags/marshalling.h>
#include <turbo/flags/reflection.h>
#include <turbo/flags/usage_config.h>
#include <turbo/numeric/int128.h>
#include <turbo/strings/match.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/string_view.h>
#include <turbo/time/clock.h>
#include <turbo/time/time.h>
#include <turbo/types/optional.h>

TURBO_DECLARE_FLAG(int64_t, mistyped_int_flag);
TURBO_DECLARE_FLAG(std::vector<std::string>, mistyped_string_flag);

namespace {

namespace flags = turbo::flags_internal;

std::string TestHelpMsg() { return "dynamic help"; }
#if defined(_MSC_VER) && !defined(__clang__)
std::string TestLiteralHelpMsg() { return "literal help"; }
#endif
template <typename T>
void TestMakeDflt(void* dst) {
  new (dst) T{};
}
void TestCallback() {}

struct UDT {
  UDT() = default;
  UDT(const UDT&) = default;
  UDT& operator=(const UDT&) = default;
};
bool turbo_parse_flag(turbo::string_view, UDT*, std::string*) { return true; }
std::string turbo_unparse_flag(const UDT&) { return ""; }

class FlagTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    // Install a function to normalize filenames before this test is run.
    turbo::FlagsUsageConfig default_config;
    default_config.normalize_filename = &FlagTest::NormalizeFileName;
    turbo::SetFlagsUsageConfig(default_config);
  }

 private:
  static std::string NormalizeFileName(turbo::string_view fname) {
#ifdef _WIN32
    std::string normalized(fname);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    fname = normalized;
#endif
    return std::string(fname);
  }
  turbo::FlagSaver flag_saver_;
};

struct S1 {
  S1() = default;
  S1(const S1&) = default;
  int32_t f1;
  int64_t f2;
};

struct S2 {
  S2() = default;
  S2(const S2&) = default;
  int64_t f1;
  double f2;
};

TEST_F(FlagTest, Traits) {
  EXPECT_EQ(flags::StorageKind<int>(),
            flags::FlagValueStorageKind::kValueAndInitBit);
  EXPECT_EQ(flags::StorageKind<bool>(),
            flags::FlagValueStorageKind::kValueAndInitBit);
  EXPECT_EQ(flags::StorageKind<double>(),
            flags::FlagValueStorageKind::kOneWordAtomic);
  EXPECT_EQ(flags::StorageKind<int64_t>(),
            flags::FlagValueStorageKind::kOneWordAtomic);

  EXPECT_EQ(flags::StorageKind<S1>(),
            flags::FlagValueStorageKind::kSequenceLocked);
  EXPECT_EQ(flags::StorageKind<S2>(),
            flags::FlagValueStorageKind::kSequenceLocked);
// Make sure turbo::Duration uses the sequence-locked code path. MSVC 2015
// doesn't consider turbo::Duration to be trivially-copyable so we just
// restrict this to clang as it seems to be a well-behaved compiler.
#ifdef __clang__
  EXPECT_EQ(flags::StorageKind<turbo::Duration>(),
            flags::FlagValueStorageKind::kSequenceLocked);
#endif

  EXPECT_EQ(flags::StorageKind<std::string>(),
            flags::FlagValueStorageKind::kAlignedBuffer);
  EXPECT_EQ(flags::StorageKind<std::vector<std::string>>(),
            flags::FlagValueStorageKind::kAlignedBuffer);

  EXPECT_EQ(flags::StorageKind<turbo::int128>(),
            flags::FlagValueStorageKind::kSequenceLocked);
  EXPECT_EQ(flags::StorageKind<turbo::uint128>(),
            flags::FlagValueStorageKind::kSequenceLocked);
}

// --------------------------------------------------------------------

constexpr flags::FlagHelpArg help_arg{flags::FlagHelpMsg("literal help"),
                                      flags::FlagHelpKind::kLiteral};

using String = std::string;
using int128 = turbo::int128;
using uint128 = turbo::uint128;

#define DEFINE_CONSTRUCTED_FLAG(T, dflt, dflt_kind)                        \
  constexpr flags::FlagDefaultArg f1default##T{                            \
      flags::FlagDefaultSrc{dflt}, flags::FlagDefaultKind::dflt_kind};     \
  constexpr turbo::Flag<T> f1##T{"f1", "file", help_arg, f1default##T};     \
  TURBO_CONST_INIT turbo::Flag<T> f2##T {                                    \
    "f2", "file",                                                          \
        {flags::FlagHelpMsg(&TestHelpMsg), flags::FlagHelpKind::kGenFunc}, \
        flags::FlagDefaultArg {                                            \
      flags::FlagDefaultSrc(&TestMakeDflt<T>),                             \
          flags::FlagDefaultKind::kGenFunc                                 \
    }                                                                      \
  }

DEFINE_CONSTRUCTED_FLAG(bool, true, kOneWord);
DEFINE_CONSTRUCTED_FLAG(int16_t, 1, kOneWord);
DEFINE_CONSTRUCTED_FLAG(uint16_t, 2, kOneWord);
DEFINE_CONSTRUCTED_FLAG(int32_t, 3, kOneWord);
DEFINE_CONSTRUCTED_FLAG(uint32_t, 4, kOneWord);
DEFINE_CONSTRUCTED_FLAG(int64_t, 5, kOneWord);
DEFINE_CONSTRUCTED_FLAG(uint64_t, 6, kOneWord);
DEFINE_CONSTRUCTED_FLAG(float, 7.8, kOneWord);
DEFINE_CONSTRUCTED_FLAG(double, 9.10, kOneWord);
DEFINE_CONSTRUCTED_FLAG(String, &TestMakeDflt<String>, kGenFunc);
DEFINE_CONSTRUCTED_FLAG(UDT, &TestMakeDflt<UDT>, kGenFunc);
DEFINE_CONSTRUCTED_FLAG(int128, 13, kGenFunc);
DEFINE_CONSTRUCTED_FLAG(uint128, 14, kGenFunc);

template <typename T>
bool TestConstructionFor(const turbo::Flag<T>& f1, turbo::Flag<T>& f2) {
  EXPECT_EQ(turbo::GetFlagReflectionHandle(f1).Name(), "f1");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(f1).Help(), "literal help");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(f1).Filename(), "file");

  flags::FlagRegistrar<T, false>(TURBO_FLAG_IMPL_FLAG_PTR(f2), nullptr)
      .OnUpdate(TestCallback);

  EXPECT_EQ(turbo::GetFlagReflectionHandle(f2).Name(), "f2");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(f2).Help(), "dynamic help");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(f2).Filename(), "file");

  return true;
}

#define TEST_CONSTRUCTED_FLAG(T) TestConstructionFor(f1##T, f2##T);

TEST_F(FlagTest, TestConstruction) {
  TEST_CONSTRUCTED_FLAG(bool);
  TEST_CONSTRUCTED_FLAG(int16_t);
  TEST_CONSTRUCTED_FLAG(uint16_t);
  TEST_CONSTRUCTED_FLAG(int32_t);
  TEST_CONSTRUCTED_FLAG(uint32_t);
  TEST_CONSTRUCTED_FLAG(int64_t);
  TEST_CONSTRUCTED_FLAG(uint64_t);
  TEST_CONSTRUCTED_FLAG(float);
  TEST_CONSTRUCTED_FLAG(double);
  TEST_CONSTRUCTED_FLAG(String);
  TEST_CONSTRUCTED_FLAG(UDT);
  TEST_CONSTRUCTED_FLAG(int128);
  TEST_CONSTRUCTED_FLAG(uint128);
}

// --------------------------------------------------------------------

}  // namespace

TURBO_DECLARE_FLAG(bool, test_flag_01);
TURBO_DECLARE_FLAG(int, test_flag_02);
TURBO_DECLARE_FLAG(int16_t, test_flag_03);
TURBO_DECLARE_FLAG(uint16_t, test_flag_04);
TURBO_DECLARE_FLAG(int32_t, test_flag_05);
TURBO_DECLARE_FLAG(uint32_t, test_flag_06);
TURBO_DECLARE_FLAG(int64_t, test_flag_07);
TURBO_DECLARE_FLAG(uint64_t, test_flag_08);
TURBO_DECLARE_FLAG(double, test_flag_09);
TURBO_DECLARE_FLAG(float, test_flag_10);
TURBO_DECLARE_FLAG(std::string, test_flag_11);
TURBO_DECLARE_FLAG(turbo::Duration, test_flag_12);
TURBO_DECLARE_FLAG(turbo::int128, test_flag_13);
TURBO_DECLARE_FLAG(turbo::uint128, test_flag_14);

namespace {

TEST_F(FlagTest, TestFlagDeclaration) {
#if TURBO_FLAGS_STRIP_NAMES
  GTEST_SKIP() << "This test requires flag names to be present";
#endif
  // test that we can access flag objects.
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_01).Name(),
            "test_flag_01");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_02).Name(),
            "test_flag_02");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_03).Name(),
            "test_flag_03");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_04).Name(),
            "test_flag_04");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_05).Name(),
            "test_flag_05");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_06).Name(),
            "test_flag_06");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_07).Name(),
            "test_flag_07");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_08).Name(),
            "test_flag_08");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_09).Name(),
            "test_flag_09");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_10).Name(),
            "test_flag_10");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_11).Name(),
            "test_flag_11");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_12).Name(),
            "test_flag_12");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_13).Name(),
            "test_flag_13");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_14).Name(),
            "test_flag_14");
}

}  // namespace

#if TURBO_FLAGS_STRIP_NAMES
// The intent of this helper struct and an expression below is to make sure that
// in the configuration where TURBO_FLAGS_STRIP_NAMES=1 registrar construction
// (in cases of of no Tail calls like OnUpdate) is constexpr and thus can and
// should be completely optimized away, thus avoiding the cost/overhead of
// static initializers.
struct VerifyConsteval {
  friend consteval flags::FlagRegistrarEmpty operator+(
      flags::FlagRegistrarEmpty, VerifyConsteval) {
    return {};
  }
};

TURBO_FLAG(int, test_registrar_const_init, 0, "") + VerifyConsteval();
#endif

// --------------------------------------------------------------------

TURBO_FLAG(bool, test_flag_01, true, "test flag 01");
TURBO_FLAG(int, test_flag_02, 1234, "test flag 02");
TURBO_FLAG(int16_t, test_flag_03, -34, "test flag 03");
TURBO_FLAG(uint16_t, test_flag_04, 189, "test flag 04");
TURBO_FLAG(int32_t, test_flag_05, 10765, "test flag 05");
TURBO_FLAG(uint32_t, test_flag_06, 40000, "test flag 06");
TURBO_FLAG(int64_t, test_flag_07, -1234567, "test flag 07");
TURBO_FLAG(uint64_t, test_flag_08, 9876543, "test flag 08");
TURBO_FLAG(double, test_flag_09, -9.876e-50, "test flag 09");
TURBO_FLAG(float, test_flag_10, 1.234e12f, "test flag 10");
TURBO_FLAG(std::string, test_flag_11, "", "test flag 11");
TURBO_FLAG(turbo::Duration, test_flag_12, turbo::Minutes(10), "test flag 12");
TURBO_FLAG(turbo::int128, test_flag_13, turbo::MakeInt128(-1, 0), "test flag 13");
TURBO_FLAG(turbo::uint128, test_flag_14, turbo::MakeUint128(0, 0xFFFAAABBBCCCDDD),
          "test flag 14");

namespace {

TEST_F(FlagTest, TestFlagDefinition) {
#if TURBO_FLAGS_STRIP_NAMES
  GTEST_SKIP() << "This test requires flag names to be present";
#endif
  turbo::string_view expected_file_name = "turbo/flags/flag_test.cc";

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_01).Name(),
            "test_flag_01");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_01).Help(),
            "test flag 01");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_01).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_01).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_02).Name(),
            "test_flag_02");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_02).Help(),
            "test flag 02");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_02).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_02).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_03).Name(),
            "test_flag_03");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_03).Help(),
            "test flag 03");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_03).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_03).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_04).Name(),
            "test_flag_04");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_04).Help(),
            "test flag 04");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_04).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_04).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_05).Name(),
            "test_flag_05");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_05).Help(),
            "test flag 05");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_05).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_05).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_06).Name(),
            "test_flag_06");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_06).Help(),
            "test flag 06");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_06).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_06).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_07).Name(),
            "test_flag_07");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_07).Help(),
            "test flag 07");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_07).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_07).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_08).Name(),
            "test_flag_08");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_08).Help(),
            "test flag 08");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_08).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_08).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_09).Name(),
            "test_flag_09");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_09).Help(),
            "test flag 09");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_09).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_09).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_10).Name(),
            "test_flag_10");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_10).Help(),
            "test flag 10");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_10).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_10).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_11).Name(),
            "test_flag_11");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_11).Help(),
            "test flag 11");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_11).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_11).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_12).Name(),
            "test_flag_12");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_12).Help(),
            "test flag 12");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_12).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_12).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_13).Name(),
            "test_flag_13");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_13).Help(),
            "test flag 13");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_13).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_13).Filename();

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_14).Name(),
            "test_flag_14");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_14).Help(),
            "test flag 14");
  EXPECT_TRUE(turbo::EndsWith(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_14).Filename(),
      expected_file_name))
      << turbo::GetFlagReflectionHandle(FLAGS_test_flag_14).Filename();
}

// --------------------------------------------------------------------

TEST_F(FlagTest, TestDefault) {
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_01).DefaultValue(),
            "true");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_02).DefaultValue(),
            "1234");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_03).DefaultValue(),
            "-34");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_04).DefaultValue(),
            "189");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_05).DefaultValue(),
            "10765");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_06).DefaultValue(),
            "40000");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_07).DefaultValue(),
            "-1234567");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_08).DefaultValue(),
            "9876543");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_09).DefaultValue(),
            "-9.876e-50");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_10).DefaultValue(),
            "1.234e+12");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_11).DefaultValue(),
            "");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_12).DefaultValue(),
            "10m");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_13).DefaultValue(),
            "-18446744073709551616");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_14).DefaultValue(),
            "1152827684197027293");

  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_01).CurrentValue(),
            "true");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_02).CurrentValue(),
            "1234");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_03).CurrentValue(),
            "-34");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_04).CurrentValue(),
            "189");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_05).CurrentValue(),
            "10765");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_06).CurrentValue(),
            "40000");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_07).CurrentValue(),
            "-1234567");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_08).CurrentValue(),
            "9876543");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_09).CurrentValue(),
            "-9.876e-50");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_10).CurrentValue(),
            "1.234e+12");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_11).CurrentValue(),
            "");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_12).CurrentValue(),
            "10m");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_13).CurrentValue(),
            "-18446744073709551616");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_14).CurrentValue(),
            "1152827684197027293");

  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_01), true);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_02), 1234);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_03), -34);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_04), 189);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_05), 10765);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_06), 40000);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_07), -1234567);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 9876543);
  EXPECT_NEAR(turbo::GetFlag(FLAGS_test_flag_09), -9.876e-50, 1e-55);
  EXPECT_NEAR(turbo::GetFlag(FLAGS_test_flag_10), 1.234e12f, 1e5f);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_11), "");
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_12), turbo::Minutes(10));
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_13), turbo::MakeInt128(-1, 0));
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_14),
            turbo::MakeUint128(0, 0xFFFAAABBBCCCDDD));
}

// --------------------------------------------------------------------

struct NonTriviallyCopyableAggregate {
  NonTriviallyCopyableAggregate() = default;
  NonTriviallyCopyableAggregate(const NonTriviallyCopyableAggregate& rhs)
      : value(rhs.value) {}
  NonTriviallyCopyableAggregate& operator=(
      const NonTriviallyCopyableAggregate& rhs) {
    value = rhs.value;
    return *this;
  }

  int value;
};
bool turbo_parse_flag(turbo::string_view src, NonTriviallyCopyableAggregate* f,
                   std::string* e) {
  return turbo::ParseFlag(src, &f->value, e);
}
std::string turbo_unparse_flag(const NonTriviallyCopyableAggregate& ntc) {
  return turbo::StrCat(ntc.value);
}

bool operator==(const NonTriviallyCopyableAggregate& ntc1,
                const NonTriviallyCopyableAggregate& ntc2) {
  return ntc1.value == ntc2.value;
}

}  // namespace

TURBO_FLAG(bool, test_flag_eb_01, {}, "");
TURBO_FLAG(int32_t, test_flag_eb_02, {}, "");
TURBO_FLAG(int64_t, test_flag_eb_03, {}, "");
TURBO_FLAG(double, test_flag_eb_04, {}, "");
TURBO_FLAG(std::string, test_flag_eb_05, {}, "");
TURBO_FLAG(NonTriviallyCopyableAggregate, test_flag_eb_06, {}, "");

namespace {

TEST_F(FlagTest, TestEmptyBracesDefault) {
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_eb_01).DefaultValue(),
            "false");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_eb_02).DefaultValue(),
            "0");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_eb_03).DefaultValue(),
            "0");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_eb_04).DefaultValue(),
            "0");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_eb_05).DefaultValue(),
            "");
  EXPECT_EQ(turbo::GetFlagReflectionHandle(FLAGS_test_flag_eb_06).DefaultValue(),
            "0");

  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_eb_01), false);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_eb_02), 0);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_eb_03), 0);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_eb_04), 0.0);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_eb_05), "");
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_eb_06),
            NonTriviallyCopyableAggregate{});
}

// --------------------------------------------------------------------

TEST_F(FlagTest, TestGetSet) {
  turbo::SetFlag(&FLAGS_test_flag_01, false);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_01), false);

  turbo::SetFlag(&FLAGS_test_flag_02, 321);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_02), 321);

  turbo::SetFlag(&FLAGS_test_flag_03, 67);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_03), 67);

  turbo::SetFlag(&FLAGS_test_flag_04, 1);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_04), 1);

  turbo::SetFlag(&FLAGS_test_flag_05, -908);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_05), -908);

  turbo::SetFlag(&FLAGS_test_flag_06, 4001);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_06), 4001);

  turbo::SetFlag(&FLAGS_test_flag_07, -23456);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_07), -23456);

  turbo::SetFlag(&FLAGS_test_flag_08, 975310);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 975310);

  turbo::SetFlag(&FLAGS_test_flag_09, 1.00001);
  EXPECT_NEAR(turbo::GetFlag(FLAGS_test_flag_09), 1.00001, 1e-10);

  turbo::SetFlag(&FLAGS_test_flag_10, -3.54f);
  EXPECT_NEAR(turbo::GetFlag(FLAGS_test_flag_10), -3.54f, 1e-6f);

  turbo::SetFlag(&FLAGS_test_flag_11, "asdf");
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_11), "asdf");

  turbo::SetFlag(&FLAGS_test_flag_12, turbo::Seconds(110));
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_12), turbo::Seconds(110));

  turbo::SetFlag(&FLAGS_test_flag_13, turbo::MakeInt128(-1, 0));
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_13), turbo::MakeInt128(-1, 0));

  turbo::SetFlag(&FLAGS_test_flag_14, turbo::MakeUint128(0, 0xFFFAAABBBCCCDDD));
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_14),
            turbo::MakeUint128(0, 0xFFFAAABBBCCCDDD));
}

// --------------------------------------------------------------------

TEST_F(FlagTest, TestGetViaReflection) {
#if TURBO_FLAGS_STRIP_NAMES
  GTEST_SKIP() << "This test requires flag names to be present";
#endif
  auto* handle = turbo::FindCommandLineFlag("test_flag_01");
  EXPECT_EQ(*handle->TryGet<bool>(), true);
  handle = turbo::FindCommandLineFlag("test_flag_02");
  EXPECT_EQ(*handle->TryGet<int>(), 1234);
  handle = turbo::FindCommandLineFlag("test_flag_03");
  EXPECT_EQ(*handle->TryGet<int16_t>(), -34);
  handle = turbo::FindCommandLineFlag("test_flag_04");
  EXPECT_EQ(*handle->TryGet<uint16_t>(), 189);
  handle = turbo::FindCommandLineFlag("test_flag_05");
  EXPECT_EQ(*handle->TryGet<int32_t>(), 10765);
  handle = turbo::FindCommandLineFlag("test_flag_06");
  EXPECT_EQ(*handle->TryGet<uint32_t>(), 40000);
  handle = turbo::FindCommandLineFlag("test_flag_07");
  EXPECT_EQ(*handle->TryGet<int64_t>(), -1234567);
  handle = turbo::FindCommandLineFlag("test_flag_08");
  EXPECT_EQ(*handle->TryGet<uint64_t>(), 9876543);
  handle = turbo::FindCommandLineFlag("test_flag_09");
  EXPECT_NEAR(*handle->TryGet<double>(), -9.876e-50, 1e-55);
  handle = turbo::FindCommandLineFlag("test_flag_10");
  EXPECT_NEAR(*handle->TryGet<float>(), 1.234e12f, 1e5f);
  handle = turbo::FindCommandLineFlag("test_flag_11");
  EXPECT_EQ(*handle->TryGet<std::string>(), "");
  handle = turbo::FindCommandLineFlag("test_flag_12");
  EXPECT_EQ(*handle->TryGet<turbo::Duration>(), turbo::Minutes(10));
  handle = turbo::FindCommandLineFlag("test_flag_13");
  EXPECT_EQ(*handle->TryGet<turbo::int128>(), turbo::MakeInt128(-1, 0));
  handle = turbo::FindCommandLineFlag("test_flag_14");
  EXPECT_EQ(*handle->TryGet<turbo::uint128>(),
            turbo::MakeUint128(0, 0xFFFAAABBBCCCDDD));
}

// --------------------------------------------------------------------

TEST_F(FlagTest, ConcurrentSetAndGet) {
#if TURBO_FLAGS_STRIP_NAMES
  GTEST_SKIP() << "This test requires flag names to be present";
#endif
  static constexpr int kNumThreads = 8;
  // Two arbitrary durations. One thread will concurrently flip the flag
  // between these two values, while the other threads read it and verify
  // that no other value is seen.
  static const turbo::Duration kValidDurations[] = {
      turbo::Seconds(int64_t{0x6cebf47a9b68c802}) + turbo::Nanoseconds(229702057),
      turbo::Seconds(int64_t{0x23fec0307e4e9d3}) + turbo::Nanoseconds(44555374)};
  turbo::SetFlag(&FLAGS_test_flag_12, kValidDurations[0]);

  std::atomic<bool> stop{false};
  std::vector<std::thread> threads;
  auto* handle = turbo::FindCommandLineFlag("test_flag_12");
  for (int i = 0; i < kNumThreads; i++) {
    threads.emplace_back([&]() {
      while (!stop.load(std::memory_order_relaxed)) {
        // Try loading the flag both directly and via a reflection
        // handle.
        turbo::Duration v = turbo::GetFlag(FLAGS_test_flag_12);
        EXPECT_TRUE(v == kValidDurations[0] || v == kValidDurations[1]);
        v = *handle->TryGet<turbo::Duration>();
        EXPECT_TRUE(v == kValidDurations[0] || v == kValidDurations[1]);
      }
    });
  }
  turbo::Time end_time = turbo::Now() + turbo::Seconds(1);
  int i = 0;
  while (turbo::Now() < end_time) {
    turbo::SetFlag(&FLAGS_test_flag_12,
                  kValidDurations[i++ % TURBO_ARRAYSIZE(kValidDurations)]);
  }
  stop.store(true, std::memory_order_relaxed);
  for (auto& t : threads) t.join();
}

// --------------------------------------------------------------------

int GetDflt1() { return 1; }

}  // namespace

TURBO_FLAG(int, test_int_flag_with_non_const_default, GetDflt1(),
          "test int flag non const default");
TURBO_FLAG(std::string, test_string_flag_with_non_const_default,
          turbo::StrCat("AAA", "BBB"), "test string flag non const default");

namespace {

TEST_F(FlagTest, TestNonConstexprDefault) {
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_int_flag_with_non_const_default), 1);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_string_flag_with_non_const_default),
            "AAABBB");
}

// --------------------------------------------------------------------

}  // namespace

TURBO_FLAG(bool, test_flag_with_non_const_help, true,
          turbo::StrCat("test ", "flag ", "non const help"));

namespace {

#if !TURBO_FLAGS_STRIP_HELP
TEST_F(FlagTest, TestNonConstexprHelp) {
  EXPECT_EQ(
      turbo::GetFlagReflectionHandle(FLAGS_test_flag_with_non_const_help).Help(),
      "test flag non const help");
}
#endif  //! TURBO_FLAGS_STRIP_HELP

// --------------------------------------------------------------------

int cb_test_value = -1;
void TestFlagCB();

}  // namespace

TURBO_FLAG(int, test_flag_with_cb, 100, "").OnUpdate(TestFlagCB);

TURBO_FLAG(int, test_flag_with_lambda_cb, 200, "").OnUpdate([]() {
  cb_test_value = turbo::GetFlag(FLAGS_test_flag_with_lambda_cb) +
                  turbo::GetFlag(FLAGS_test_flag_with_cb);
});

namespace {

void TestFlagCB() { cb_test_value = turbo::GetFlag(FLAGS_test_flag_with_cb); }

// Tests side-effects of callback invocation.
TEST_F(FlagTest, CallbackInvocation) {
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_with_cb), 100);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_with_lambda_cb), 200);
  EXPECT_EQ(cb_test_value, 300);

  turbo::SetFlag(&FLAGS_test_flag_with_cb, 1);
  EXPECT_EQ(cb_test_value, 1);

  turbo::SetFlag(&FLAGS_test_flag_with_lambda_cb, 3);
  EXPECT_EQ(cb_test_value, 4);
}

// --------------------------------------------------------------------

struct CustomUDT {
  CustomUDT() : a(1), b(1) {}
  CustomUDT(int a_, int b_) : a(a_), b(b_) {}

  friend bool operator==(const CustomUDT& f1, const CustomUDT& f2) {
    return f1.a == f2.a && f1.b == f2.b;
  }

  int a;
  int b;
};
bool turbo_parse_flag(turbo::string_view in, CustomUDT* f, std::string*) {
  std::vector<turbo::string_view> parts =
      turbo::StrSplit(in, ':', turbo::SkipWhitespace());

  if (parts.size() != 2) return false;

  if (!turbo::SimpleAtoi(parts[0], &f->a)) return false;

  if (!turbo::SimpleAtoi(parts[1], &f->b)) return false;

  return true;
}
std::string turbo_unparse_flag(const CustomUDT& f) {
  return turbo::StrCat(f.a, ":", f.b);
}

}  // namespace

TURBO_FLAG(CustomUDT, test_flag_custom_udt, CustomUDT(), "test flag custom UDT");

namespace {

TEST_F(FlagTest, TestCustomUDT) {
  EXPECT_EQ(flags::StorageKind<CustomUDT>(),
            flags::FlagValueStorageKind::kOneWordAtomic);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_custom_udt), CustomUDT(1, 1));
  turbo::SetFlag(&FLAGS_test_flag_custom_udt, CustomUDT(2, 3));
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_custom_udt), CustomUDT(2, 3));
}

// MSVC produces link error on the type mismatch.
// Linux does not have build errors and validations work as expected.
#if !defined(_WIN32) && GTEST_HAS_DEATH_TEST
using FlagDeathTest = FlagTest;

TEST_F(FlagDeathTest, TestTypeMismatchValidations) {
#if TURBO_FLAGS_STRIP_NAMES
  GTEST_SKIP() << "This test requires flag names to be present";
#endif
#if !defined(NDEBUG)
  EXPECT_DEATH_IF_SUPPORTED(
      static_cast<void>(turbo::GetFlag(FLAGS_mistyped_int_flag)),
      "Flag 'mistyped_int_flag' is defined as one type and declared "
      "as another");
  EXPECT_DEATH_IF_SUPPORTED(
      static_cast<void>(turbo::GetFlag(FLAGS_mistyped_string_flag)),
      "Flag 'mistyped_string_flag' is defined as one type and "
      "declared as another");
#endif

  EXPECT_DEATH_IF_SUPPORTED(
      turbo::SetFlag(&FLAGS_mistyped_int_flag, 1),
      "Flag 'mistyped_int_flag' is defined as one type and declared "
      "as another");
  EXPECT_DEATH_IF_SUPPORTED(
      turbo::SetFlag(&FLAGS_mistyped_string_flag, std::vector<std::string>{}),
      "Flag 'mistyped_string_flag' is defined as one type and declared as "
      "another");
}

#endif

// --------------------------------------------------------------------

// A contrived type that offers implicit and explicit conversion from specific
// source types.
struct ConversionTestVal {
  ConversionTestVal() = default;
  explicit ConversionTestVal(int a_in) : a(a_in) {}

  enum class ViaImplicitConv { kTen = 10, kEleven };
  // NOLINTNEXTLINE
  ConversionTestVal(ViaImplicitConv from) : a(static_cast<int>(from)) {}

  int a;
};

bool turbo_parse_flag(turbo::string_view in, ConversionTestVal* val_out,
                   std::string*) {
  if (!turbo::SimpleAtoi(in, &val_out->a)) {
    return false;
  }
  return true;
}
std::string turbo_unparse_flag(const ConversionTestVal& val) {
  return turbo::StrCat(val.a);
}

}  // namespace

// Flag default values can be specified with a value that converts to the flag
// value type implicitly.
TURBO_FLAG(ConversionTestVal, test_flag_implicit_conv,
          ConversionTestVal::ViaImplicitConv::kTen,
          "test flag init via implicit conversion");

namespace {

TEST_F(FlagTest, CanSetViaImplicitConversion) {
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_implicit_conv).a, 10);
  turbo::SetFlag(&FLAGS_test_flag_implicit_conv,
                ConversionTestVal::ViaImplicitConv::kEleven);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_implicit_conv).a, 11);
}

// --------------------------------------------------------------------

struct NonDfltConstructible {
 public:
  // This constructor tests that we can initialize the flag with int value
  NonDfltConstructible(int i) : value(i) {}  // NOLINT

  // This constructor tests that we can't initialize the flag with char value
  // but can with explicitly constructed NonDfltConstructible.
  explicit NonDfltConstructible(char c) : value(100 + static_cast<int>(c)) {}

  int value;
};

bool turbo_parse_flag(turbo::string_view in, NonDfltConstructible* ndc_out,
                   std::string*) {
  return turbo::SimpleAtoi(in, &ndc_out->value);
}
std::string turbo_unparse_flag(const NonDfltConstructible& ndc) {
  return turbo::StrCat(ndc.value);
}

}  // namespace

TURBO_FLAG(NonDfltConstructible, ndc_flag1, NonDfltConstructible('1'),
          "Flag with non default constructible type");
TURBO_FLAG(NonDfltConstructible, ndc_flag2, 0,
          "Flag with non default constructible type");

namespace {

TEST_F(FlagTest, TestNonDefaultConstructibleType) {
  EXPECT_EQ(turbo::GetFlag(FLAGS_ndc_flag1).value, '1' + 100);
  EXPECT_EQ(turbo::GetFlag(FLAGS_ndc_flag2).value, 0);

  turbo::SetFlag(&FLAGS_ndc_flag1, NonDfltConstructible('A'));
  turbo::SetFlag(&FLAGS_ndc_flag2, 25);

  EXPECT_EQ(turbo::GetFlag(FLAGS_ndc_flag1).value, 'A' + 100);
  EXPECT_EQ(turbo::GetFlag(FLAGS_ndc_flag2).value, 25);
}

}  // namespace

// --------------------------------------------------------------------

TURBO_RETIRED_FLAG(bool, old_bool_flag, true, "old descr");
TURBO_RETIRED_FLAG(int, old_int_flag, (int)std::sqrt(10), "old descr");
TURBO_RETIRED_FLAG(std::string, old_str_flag, "", turbo::StrCat("old ", "descr"));

namespace {

bool initialization_order_fiasco_test TURBO_ATTRIBUTE_UNUSED = [] {
  // Iterate over all the flags during static initialization.
  // This should not trigger ASan's initialization-order-fiasco.
  auto* handle1 = turbo::FindCommandLineFlag("flag_on_separate_file");
  auto* handle2 = turbo::FindCommandLineFlag("retired_flag_on_separate_file");
  if (handle1 != nullptr && handle2 != nullptr) {
    return handle1->Name() == handle2->Name();
  }
  return true;
}();

TEST_F(FlagTest, TestRetiredFlagRegistration) {
  auto* handle = turbo::FindCommandLineFlag("old_bool_flag");
  EXPECT_TRUE(handle->IsOfType<bool>());
  EXPECT_TRUE(handle->IsRetired());
  handle = turbo::FindCommandLineFlag("old_int_flag");
  EXPECT_TRUE(handle->IsOfType<int>());
  EXPECT_TRUE(handle->IsRetired());
  handle = turbo::FindCommandLineFlag("old_str_flag");
  EXPECT_TRUE(handle->IsOfType<std::string>());
  EXPECT_TRUE(handle->IsRetired());
}

}  // namespace

// --------------------------------------------------------------------

namespace {

// User-defined type with small alignment, but size exceeding 16.
struct SmallAlignUDT {
  SmallAlignUDT() : c('A'), s(12) {}
  char c;
  int16_t s;
  char bytes[14];
};

bool turbo_parse_flag(turbo::string_view, SmallAlignUDT*, std::string*) {
  return true;
}
std::string turbo_unparse_flag(const SmallAlignUDT&) { return ""; }

// User-defined type with small size, but not trivially copyable.
struct NonTriviallyCopyableUDT {
  NonTriviallyCopyableUDT() : c('A') {}
  NonTriviallyCopyableUDT(const NonTriviallyCopyableUDT& rhs) : c(rhs.c) {}
  NonTriviallyCopyableUDT& operator=(const NonTriviallyCopyableUDT& rhs) {
    c = rhs.c;
    return *this;
  }

  char c;
};

bool turbo_parse_flag(turbo::string_view, NonTriviallyCopyableUDT*, std::string*) {
  return true;
}
std::string turbo_unparse_flag(const NonTriviallyCopyableUDT&) { return ""; }

}  // namespace

TURBO_FLAG(SmallAlignUDT, test_flag_sa_udt, {}, "help");
TURBO_FLAG(NonTriviallyCopyableUDT, test_flag_ntc_udt, {}, "help");

namespace {

TEST_F(FlagTest, TestSmallAlignUDT) {
  SmallAlignUDT value = turbo::GetFlag(FLAGS_test_flag_sa_udt);
  EXPECT_EQ(value.c, 'A');
  EXPECT_EQ(value.s, 12);

  value.c = 'B';
  value.s = 45;
  turbo::SetFlag(&FLAGS_test_flag_sa_udt, value);
  value = turbo::GetFlag(FLAGS_test_flag_sa_udt);
  EXPECT_EQ(value.c, 'B');
  EXPECT_EQ(value.s, 45);
}

TEST_F(FlagTest, TestNonTriviallyCopyableUDT) {
  NonTriviallyCopyableUDT value = turbo::GetFlag(FLAGS_test_flag_ntc_udt);
  EXPECT_EQ(value.c, 'A');

  value.c = 'B';
  turbo::SetFlag(&FLAGS_test_flag_ntc_udt, value);
  value = turbo::GetFlag(FLAGS_test_flag_ntc_udt);
  EXPECT_EQ(value.c, 'B');
}

}  // namespace

// --------------------------------------------------------------------

namespace {

enum TestE { A = 1, B = 2, C = 3 };

struct EnumWrapper {
  EnumWrapper() : e(A) {}

  TestE e;
};

bool turbo_parse_flag(turbo::string_view, EnumWrapper*, std::string*) {
  return true;
}
std::string turbo_unparse_flag(const EnumWrapper&) { return ""; }

}  // namespace

TURBO_FLAG(EnumWrapper, test_enum_wrapper_flag, {}, "help");

TEST_F(FlagTest, TesTypeWrappingEnum) {
  EnumWrapper value = turbo::GetFlag(FLAGS_test_enum_wrapper_flag);
  EXPECT_EQ(value.e, A);

  value.e = B;
  turbo::SetFlag(&FLAGS_test_enum_wrapper_flag, value);
  value = turbo::GetFlag(FLAGS_test_enum_wrapper_flag);
  EXPECT_EQ(value.e, B);
}

// This is a compile test to ensure macros are expanded within TURBO_FLAG and
// TURBO_DECLARE_FLAG.
#define FLAG_NAME_MACRO(name) prefix_##name
TURBO_DECLARE_FLAG(int, FLAG_NAME_MACRO(test_macro_named_flag));
TURBO_FLAG(int, FLAG_NAME_MACRO(test_macro_named_flag), 0,
          "Testing macro expansion within TURBO_FLAG");

TEST_F(FlagTest, MacroWithinTurboFlag) {
  EXPECT_EQ(turbo::GetFlag(FLAGS_prefix_test_macro_named_flag), 0);
  turbo::SetFlag(&FLAGS_prefix_test_macro_named_flag, 1);
  EXPECT_EQ(turbo::GetFlag(FLAGS_prefix_test_macro_named_flag), 1);
}

// --------------------------------------------------------------------

TURBO_FLAG(turbo::optional<bool>, optional_bool, turbo::nullopt, "help");
TURBO_FLAG(turbo::optional<int>, optional_int, {}, "help");
TURBO_FLAG(turbo::optional<double>, optional_double, 9.3, "help");
TURBO_FLAG(turbo::optional<std::string>, optional_string, turbo::nullopt, "help");
TURBO_FLAG(turbo::optional<turbo::Duration>, optional_duration, turbo::nullopt,
          "help");
TURBO_FLAG(turbo::optional<turbo::optional<int>>, optional_optional_int,
          turbo::nullopt, "help");
#if defined(TURBO_HAVE_STD_OPTIONAL) && !defined(TURBO_USES_STD_OPTIONAL)
TURBO_FLAG(std::optional<int64_t>, std_optional_int64, std::nullopt, "help");
#endif

namespace {

TEST_F(FlagTest, TestOptionalBool) {
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_bool).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_bool), turbo::nullopt);

  turbo::SetFlag(&FLAGS_optional_bool, false);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_bool).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_bool), false);

  turbo::SetFlag(&FLAGS_optional_bool, true);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_bool).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_bool), true);

  turbo::SetFlag(&FLAGS_optional_bool, turbo::nullopt);
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_bool).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_bool), turbo::nullopt);
}

// --------------------------------------------------------------------

TEST_F(FlagTest, TestOptionalInt) {
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_int).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_int), turbo::nullopt);

  turbo::SetFlag(&FLAGS_optional_int, 0);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_int).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_int), 0);

  turbo::SetFlag(&FLAGS_optional_int, 10);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_int).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_int), 10);

  turbo::SetFlag(&FLAGS_optional_int, turbo::nullopt);
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_int).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_int), turbo::nullopt);
}

// --------------------------------------------------------------------

TEST_F(FlagTest, TestOptionalDouble) {
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_double).has_value());
  EXPECT_DOUBLE_EQ(*turbo::GetFlag(FLAGS_optional_double), 9.3);

  turbo::SetFlag(&FLAGS_optional_double, 0.0);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_double).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_double), 0.0);

  turbo::SetFlag(&FLAGS_optional_double, 1.234);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_double).has_value());
  EXPECT_DOUBLE_EQ(*turbo::GetFlag(FLAGS_optional_double), 1.234);

  turbo::SetFlag(&FLAGS_optional_double, turbo::nullopt);
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_double).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_double), turbo::nullopt);
}

// --------------------------------------------------------------------

TEST_F(FlagTest, TestOptionalString) {
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_string).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_string), turbo::nullopt);

  // Setting optional string to "" leads to undefined behavior.

  turbo::SetFlag(&FLAGS_optional_string, " ");
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_string).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_string), " ");

  turbo::SetFlag(&FLAGS_optional_string, "QWERTY");
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_string).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_string), "QWERTY");

  turbo::SetFlag(&FLAGS_optional_string, turbo::nullopt);
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_string).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_string), turbo::nullopt);
}

// --------------------------------------------------------------------

TEST_F(FlagTest, TestOptionalDuration) {
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_duration).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_duration), turbo::nullopt);

  turbo::SetFlag(&FLAGS_optional_duration, turbo::ZeroDuration());
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_duration).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_duration), turbo::Seconds(0));

  turbo::SetFlag(&FLAGS_optional_duration, turbo::Hours(3));
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_duration).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_duration), turbo::Hours(3));

  turbo::SetFlag(&FLAGS_optional_duration, turbo::nullopt);
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_duration).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_duration), turbo::nullopt);
}

// --------------------------------------------------------------------

TEST_F(FlagTest, TestOptionalOptional) {
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_optional_int).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_optional_int), turbo::nullopt);

  turbo::optional<int> nullint{turbo::nullopt};

  turbo::SetFlag(&FLAGS_optional_optional_int, nullint);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_optional_int).has_value());
  EXPECT_NE(turbo::GetFlag(FLAGS_optional_optional_int), nullint);
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_optional_int),
            turbo::optional<turbo::optional<int>>{nullint});

  turbo::SetFlag(&FLAGS_optional_optional_int, 0);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_optional_int).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_optional_int), 0);

  turbo::SetFlag(&FLAGS_optional_optional_int, turbo::optional<int>{0});
  EXPECT_TRUE(turbo::GetFlag(FLAGS_optional_optional_int).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_optional_int), 0);
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_optional_int), turbo::optional<int>{0});

  turbo::SetFlag(&FLAGS_optional_optional_int, turbo::nullopt);
  EXPECT_FALSE(turbo::GetFlag(FLAGS_optional_optional_int).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_optional_optional_int), turbo::nullopt);
}

// --------------------------------------------------------------------

#if defined(TURBO_HAVE_STD_OPTIONAL) && !defined(TURBO_USES_STD_OPTIONAL)

TEST_F(FlagTest, TestStdOptional) {
  EXPECT_FALSE(turbo::GetFlag(FLAGS_std_optional_int64).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_std_optional_int64), std::nullopt);

  turbo::SetFlag(&FLAGS_std_optional_int64, 0);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_std_optional_int64).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_std_optional_int64), 0);

  turbo::SetFlag(&FLAGS_std_optional_int64, 0xFFFFFFFFFF16);
  EXPECT_TRUE(turbo::GetFlag(FLAGS_std_optional_int64).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_std_optional_int64), 0xFFFFFFFFFF16);

  turbo::SetFlag(&FLAGS_std_optional_int64, std::nullopt);
  EXPECT_FALSE(turbo::GetFlag(FLAGS_std_optional_int64).has_value());
  EXPECT_EQ(turbo::GetFlag(FLAGS_std_optional_int64), std::nullopt);
}

// --------------------------------------------------------------------

#endif

}  // namespace
