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

#include <turbo/strings/internal/str_format/arg.h>

#include <limits>
#include <string>
#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/strings/str_format.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace str_format_internal {
namespace {

class FormatArgImplTest : public ::testing::Test {
 public:
  enum Color { kRed, kGreen, kBlue };

  static const char *hi() { return "hi"; }

  struct X {};

  X x_;
};

inline FormatConvertResult<FormatConversionCharSet{}> TurboFormatConvert(
    const FormatArgImplTest::X &, const FormatConversionSpec &, FormatSink *) {
  return {false};
}

TEST_F(FormatArgImplTest, ToInt) {
  int out = 0;
  EXPECT_TRUE(FormatArgImplFriend::ToInt(FormatArgImpl(1), &out));
  EXPECT_EQ(1, out);
  EXPECT_TRUE(FormatArgImplFriend::ToInt(FormatArgImpl(-1), &out));
  EXPECT_EQ(-1, out);
  EXPECT_TRUE(
      FormatArgImplFriend::ToInt(FormatArgImpl(static_cast<char>(64)), &out));
  EXPECT_EQ(64, out);
  EXPECT_TRUE(FormatArgImplFriend::ToInt(
      FormatArgImpl(static_cast<unsigned long long>(123456)), &out));  // NOLINT
  EXPECT_EQ(123456, out);
  EXPECT_TRUE(FormatArgImplFriend::ToInt(
      FormatArgImpl(static_cast<unsigned long long>(  // NOLINT
                        std::numeric_limits<int>::max()) +
                    1),
      &out));
  EXPECT_EQ(std::numeric_limits<int>::max(), out);
  EXPECT_TRUE(FormatArgImplFriend::ToInt(
      FormatArgImpl(static_cast<long long>(  // NOLINT
                        std::numeric_limits<int>::min()) -
                    10),
      &out));
  EXPECT_EQ(std::numeric_limits<int>::min(), out);
  EXPECT_TRUE(FormatArgImplFriend::ToInt(FormatArgImpl(false), &out));
  EXPECT_EQ(0, out);
  EXPECT_TRUE(FormatArgImplFriend::ToInt(FormatArgImpl(true), &out));
  EXPECT_EQ(1, out);
  EXPECT_FALSE(FormatArgImplFriend::ToInt(FormatArgImpl(2.2), &out));
  EXPECT_FALSE(FormatArgImplFriend::ToInt(FormatArgImpl(3.2f), &out));
  EXPECT_FALSE(FormatArgImplFriend::ToInt(
      FormatArgImpl(static_cast<int *>(nullptr)), &out));
  EXPECT_FALSE(FormatArgImplFriend::ToInt(FormatArgImpl(hi()), &out));
  EXPECT_FALSE(FormatArgImplFriend::ToInt(FormatArgImpl("hi"), &out));
  EXPECT_FALSE(FormatArgImplFriend::ToInt(FormatArgImpl(x_), &out));
  EXPECT_TRUE(FormatArgImplFriend::ToInt(FormatArgImpl(kBlue), &out));
  EXPECT_EQ(2, out);
}

extern const char kMyArray[];

TEST_F(FormatArgImplTest, CharArraysDecayToCharPtr) {
  const char* a = "";
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(a)),
            FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl("")));
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(a)),
            FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl("A")));
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(a)),
            FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl("ABC")));
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(a)),
            FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(kMyArray)));
}

extern const wchar_t kMyWCharTArray[];

TEST_F(FormatArgImplTest, WCharTArraysDecayToWCharTPtr) {
  const wchar_t* a = L"";
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(a)),
            FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(L"")));
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(a)),
            FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(L"A")));
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(a)),
            FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(L"ABC")));
  EXPECT_EQ(
      FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(a)),
      FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(kMyWCharTArray)));
}

TEST_F(FormatArgImplTest, OtherPtrDecayToVoidPtr) {
  auto expected = FormatArgImplFriend::GetVTablePtrForTest(
      FormatArgImpl(static_cast<void *>(nullptr)));
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(
                FormatArgImpl(static_cast<int *>(nullptr))),
            expected);
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(
                FormatArgImpl(static_cast<volatile int *>(nullptr))),
            expected);

  auto p = static_cast<void (*)()>([] {});
  EXPECT_EQ(FormatArgImplFriend::GetVTablePtrForTest(FormatArgImpl(p)),
            expected);
}

TEST_F(FormatArgImplTest, WorksWithCharArraysOfUnknownSize) {
  std::string s;
  FormatSinkImpl sink(&s);
  FormatConversionSpecImpl conv;
  FormatConversionSpecImplFriend::SetConversionChar(
      FormatConversionCharInternal::s, &conv);
  FormatConversionSpecImplFriend::SetFlags(Flags(), &conv);
  FormatConversionSpecImplFriend::SetWidth(-1, &conv);
  FormatConversionSpecImplFriend::SetPrecision(-1, &conv);
  EXPECT_TRUE(
      FormatArgImplFriend::Convert(FormatArgImpl(kMyArray), conv, &sink));
  sink.Flush();
  EXPECT_EQ("ABCDE", s);
}
const char kMyArray[] = "ABCDE";

TEST_F(FormatArgImplTest, WorksWithWCharTArraysOfUnknownSize) {
  std::string s;
  FormatSinkImpl sink(&s);
  FormatConversionSpecImpl conv;
  FormatConversionSpecImplFriend::SetConversionChar(
      FormatConversionCharInternal::s, &conv);
  FormatConversionSpecImplFriend::SetFlags(Flags(), &conv);
  FormatConversionSpecImplFriend::SetWidth(-1, &conv);
  FormatConversionSpecImplFriend::SetPrecision(-1, &conv);
  EXPECT_TRUE(
      FormatArgImplFriend::Convert(FormatArgImpl(kMyWCharTArray), conv, &sink));
  sink.Flush();
  EXPECT_EQ("ABCDE", s);
}
const wchar_t kMyWCharTArray[] = L"ABCDE";

}  // namespace
}  // namespace str_format_internal
TURBO_NAMESPACE_END
}  // namespace turbo
