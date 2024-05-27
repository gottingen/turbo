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

#include <turbo/strings/internal/ostringstream.h>

#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace {

TEST(OStringStream, IsOStream) {
  static_assert(
      std::is_base_of<std::ostream, turbo::strings_internal::OStringStream>(),
      "");
}

TEST(OStringStream, ConstructNullptr) {
  turbo::strings_internal::OStringStream strm(nullptr);
  EXPECT_EQ(nullptr, strm.str());
}

TEST(OStringStream, ConstructStr) {
  std::string s = "abc";
  {
    turbo::strings_internal::OStringStream strm(&s);
    EXPECT_EQ(&s, strm.str());
  }
  EXPECT_EQ("abc", s);
}

TEST(OStringStream, Destroy) {
  std::unique_ptr<std::string> s(new std::string);
  turbo::strings_internal::OStringStream strm(s.get());
  s.reset();
}

TEST(OStringStream, MoveConstruct) {
  std::string s = "abc";
  {
    turbo::strings_internal::OStringStream strm1(&s);
    strm1 << std::hex << 16;
    EXPECT_EQ(&s, strm1.str());
    turbo::strings_internal::OStringStream strm2(std::move(strm1));
    strm2 << 16;  // We should still be in base 16.
    EXPECT_EQ(&s, strm2.str());
  }
  EXPECT_EQ("abc1010", s);
}

TEST(OStringStream, MoveAssign) {
  std::string s = "abc";
  {
    turbo::strings_internal::OStringStream strm1(&s);
    strm1 << std::hex << 16;
    EXPECT_EQ(&s, strm1.str());
    turbo::strings_internal::OStringStream strm2(nullptr);
    strm2 = std::move(strm1);
    strm2 << 16;  // We should still be in base 16.
    EXPECT_EQ(&s, strm2.str());
  }
  EXPECT_EQ("abc1010", s);
}

TEST(OStringStream, Str) {
  std::string s1;
  turbo::strings_internal::OStringStream strm(&s1);
  const turbo::strings_internal::OStringStream& c_strm(strm);

  static_assert(std::is_same<decltype(strm.str()), std::string*>(), "");
  static_assert(std::is_same<decltype(c_strm.str()), const std::string*>(), "");

  EXPECT_EQ(&s1, strm.str());
  EXPECT_EQ(&s1, c_strm.str());

  strm.str(&s1);
  EXPECT_EQ(&s1, strm.str());
  EXPECT_EQ(&s1, c_strm.str());

  std::string s2;
  strm.str(&s2);
  EXPECT_EQ(&s2, strm.str());
  EXPECT_EQ(&s2, c_strm.str());

  strm.str(nullptr);
  EXPECT_EQ(nullptr, strm.str());
  EXPECT_EQ(nullptr, c_strm.str());
}

TEST(OStreamStream, WriteToLValue) {
  std::string s = "abc";
  {
    turbo::strings_internal::OStringStream strm(&s);
    EXPECT_EQ("abc", s);
    strm << "";
    EXPECT_EQ("abc", s);
    strm << 42;
    EXPECT_EQ("abc42", s);
    strm << 'x' << 'y';
    EXPECT_EQ("abc42xy", s);
  }
  EXPECT_EQ("abc42xy", s);
}

TEST(OStreamStream, WriteToRValue) {
  std::string s = "abc";
  turbo::strings_internal::OStringStream(&s) << "";
  EXPECT_EQ("abc", s);
  turbo::strings_internal::OStringStream(&s) << 42;
  EXPECT_EQ("abc42", s);
  turbo::strings_internal::OStringStream(&s) << 'x' << 'y';
  EXPECT_EQ("abc42xy", s);
}

}  // namespace
