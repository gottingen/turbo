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

#include <cstddef>

#include <gtest/gtest.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_format.h>
#include <turbo/strings/substitute.h>

namespace {

TEST(CharFormatting, Char) {
  const char v = 'A';

  // Desired behavior: does not compile:
  // EXPECT_EQ(turbo::StrCat(v, "B"), "AB");
  // EXPECT_EQ(turbo::StrFormat("%vB", v), "AB");

  // Legacy behavior: format as char:
  EXPECT_EQ(turbo::Substitute("$0B", v), "AB");
}

enum CharEnum : char {};
TEST(CharFormatting, CharEnum) {
  auto v = static_cast<CharEnum>('A');

  // Desired behavior: format as decimal
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");

  // Legacy behavior: format as character:

  // Some older versions of gcc behave differently in this one case
#if !defined(__GNUC__) || defined(__clang__)
  EXPECT_EQ(turbo::Substitute("$0B", v), "AB");
#endif
}

enum class CharEnumClass: char {};
TEST(CharFormatting, CharEnumClass) {
  auto v = static_cast<CharEnumClass>('A');

  // Desired behavior: format as decimal:
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");

  // Legacy behavior: format as character:
  EXPECT_EQ(turbo::Substitute("$0B", v), "AB");
}

TEST(CharFormatting, UnsignedChar) {
  const unsigned char v = 'A';

  // Desired behavior: format as decimal:
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");
  EXPECT_EQ(turbo::Substitute("$0B", v), "65B");
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");

  // Signedness check
  const unsigned char w = 255;
  EXPECT_EQ(turbo::StrCat(w, "B"), "255B");
  EXPECT_EQ(turbo::Substitute("$0B", w), "255B");
  // EXPECT_EQ(turbo::StrFormat("%vB", v), "255B");
}

TEST(CharFormatting, SignedChar) {
  const signed char v = 'A';

  // Desired behavior: format as decimal:
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");
  EXPECT_EQ(turbo::Substitute("$0B", v), "65B");
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");

  // Signedness check
  const signed char w = -128;
  EXPECT_EQ(turbo::StrCat(w, "B"), "-128B");
  EXPECT_EQ(turbo::Substitute("$0B", w), "-128B");
}

enum UnsignedCharEnum : unsigned char {};
TEST(CharFormatting, UnsignedCharEnum) {
  auto v = static_cast<UnsignedCharEnum>('A');

  // Desired behavior: format as decimal:
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");
  EXPECT_EQ(turbo::Substitute("$0B", v), "65B");
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");

  // Signedness check
  auto w = static_cast<UnsignedCharEnum>(255);
  EXPECT_EQ(turbo::StrCat(w, "B"), "255B");
  EXPECT_EQ(turbo::Substitute("$0B", w), "255B");
  EXPECT_EQ(turbo::StrFormat("%vB", w), "255B");
}

enum SignedCharEnum : signed char {};
TEST(CharFormatting, SignedCharEnum) {
  auto v = static_cast<SignedCharEnum>('A');

  // Desired behavior: format as decimal:
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");
  EXPECT_EQ(turbo::Substitute("$0B", v), "65B");
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");

  // Signedness check
  auto w = static_cast<SignedCharEnum>(-128);
  EXPECT_EQ(turbo::StrCat(w, "B"), "-128B");
  EXPECT_EQ(turbo::Substitute("$0B", w), "-128B");
  EXPECT_EQ(turbo::StrFormat("%vB", w), "-128B");
}

enum class UnsignedCharEnumClass : unsigned char {};
TEST(CharFormatting, UnsignedCharEnumClass) {
  auto v = static_cast<UnsignedCharEnumClass>('A');

  // Desired behavior: format as decimal:
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");
  EXPECT_EQ(turbo::Substitute("$0B", v), "65B");
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");

  // Signedness check
  auto w = static_cast<UnsignedCharEnumClass>(255);
  EXPECT_EQ(turbo::StrCat(w, "B"), "255B");
  EXPECT_EQ(turbo::Substitute("$0B", w), "255B");
  EXPECT_EQ(turbo::StrFormat("%vB", w), "255B");
}

enum SignedCharEnumClass : signed char {};
TEST(CharFormatting, SignedCharEnumClass) {
  auto v = static_cast<SignedCharEnumClass>('A');

  // Desired behavior: format as decimal:
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");
  EXPECT_EQ(turbo::Substitute("$0B", v), "65B");
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");

  // Signedness check
  auto w = static_cast<SignedCharEnumClass>(-128);
  EXPECT_EQ(turbo::StrCat(w, "B"), "-128B");
  EXPECT_EQ(turbo::Substitute("$0B", w), "-128B");
  EXPECT_EQ(turbo::StrFormat("%vB", w), "-128B");
}

#ifdef __cpp_lib_byte
TEST(CharFormatting, StdByte) {
  auto v = static_cast<std::byte>('A');
  // Desired behavior: format as 0xff
  // (No APIs do this today.)

  // Legacy behavior: format as decimal:
  EXPECT_EQ(turbo::StrCat(v, "B"), "65B");
  EXPECT_EQ(turbo::Substitute("$0B", v), "65B");
  EXPECT_EQ(turbo::StrFormat("%vB", v), "65B");
}
#endif  // _cpp_lib_byte

}  // namespace
