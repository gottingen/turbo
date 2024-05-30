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

#include <turbo/strings/ascii.h>

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>

#include <gtest/gtest.h>
#include <turbo/base/macros.h>
#include <turbo/strings/string_view.h>

namespace {

TEST(AsciiIsFoo, All) {
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
      EXPECT_TRUE(turbo::ascii_isalpha(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_isalpha(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if ((c >= '0' && c <= '9'))
      EXPECT_TRUE(turbo::ascii_isdigit(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_isdigit(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (turbo::ascii_isalpha(c) || turbo::ascii_isdigit(c))
      EXPECT_TRUE(turbo::ascii_isalnum(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_isalnum(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (i != '\0' && strchr(" \r\n\t\v\f", i))
      EXPECT_TRUE(turbo::ascii_isspace(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_isspace(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (i >= 32 && i < 127)
      EXPECT_TRUE(turbo::ascii_isprint(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_isprint(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (turbo::ascii_isprint(c) && !turbo::ascii_isspace(c) &&
        !turbo::ascii_isalnum(c)) {
      EXPECT_TRUE(turbo::ascii_ispunct(c)) << ": failed on " << c;
    } else {
      EXPECT_TRUE(!turbo::ascii_ispunct(c)) << ": failed on " << c;
    }
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (i == ' ' || i == '\t')
      EXPECT_TRUE(turbo::ascii_isblank(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_isblank(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (i < 32 || i == 127)
      EXPECT_TRUE(turbo::ascii_iscntrl(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_iscntrl(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (turbo::ascii_isdigit(c) || (i >= 'A' && i <= 'F') ||
        (i >= 'a' && i <= 'f')) {
      EXPECT_TRUE(turbo::ascii_isxdigit(c)) << ": failed on " << c;
    } else {
      EXPECT_TRUE(!turbo::ascii_isxdigit(c)) << ": failed on " << c;
    }
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (i > 32 && i < 127)
      EXPECT_TRUE(turbo::ascii_isgraph(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_isgraph(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (i >= 'A' && i <= 'Z')
      EXPECT_TRUE(turbo::ascii_isupper(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_isupper(c)) << ": failed on " << c;
  }
  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (i >= 'a' && i <= 'z')
      EXPECT_TRUE(turbo::ascii_islower(c)) << ": failed on " << c;
    else
      EXPECT_TRUE(!turbo::ascii_islower(c)) << ": failed on " << c;
  }
  for (unsigned char c = 0; c < 128; c++) {
    EXPECT_TRUE(turbo::ascii_isascii(c)) << ": failed on " << c;
  }
  for (int i = 128; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    EXPECT_TRUE(!turbo::ascii_isascii(c)) << ": failed on " << c;
  }
}

// Checks that turbo::ascii_isfoo returns the same value as isfoo in the C
// locale.
TEST(AsciiIsFoo, SameAsIsFoo) {
#ifndef __ANDROID__
  // temporarily change locale to C. It should already be C, but just for safety
  const char* old_locale = setlocale(LC_CTYPE, "C");
  ASSERT_TRUE(old_locale != nullptr);
#endif

  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    EXPECT_EQ(isalpha(c) != 0, turbo::ascii_isalpha(c)) << c;
    EXPECT_EQ(isdigit(c) != 0, turbo::ascii_isdigit(c)) << c;
    EXPECT_EQ(isalnum(c) != 0, turbo::ascii_isalnum(c)) << c;
    EXPECT_EQ(isspace(c) != 0, turbo::ascii_isspace(c)) << c;
    EXPECT_EQ(ispunct(c) != 0, turbo::ascii_ispunct(c)) << c;
    EXPECT_EQ(isblank(c) != 0, turbo::ascii_isblank(c)) << c;
    EXPECT_EQ(iscntrl(c) != 0, turbo::ascii_iscntrl(c)) << c;
    EXPECT_EQ(isxdigit(c) != 0, turbo::ascii_isxdigit(c)) << c;
    EXPECT_EQ(isprint(c) != 0, turbo::ascii_isprint(c)) << c;
    EXPECT_EQ(isgraph(c) != 0, turbo::ascii_isgraph(c)) << c;
    EXPECT_EQ(isupper(c) != 0, turbo::ascii_isupper(c)) << c;
    EXPECT_EQ(islower(c) != 0, turbo::ascii_islower(c)) << c;
    EXPECT_EQ(isascii(c) != 0, turbo::ascii_isascii(c)) << c;
  }

#ifndef __ANDROID__
  // restore the old locale.
  ASSERT_TRUE(setlocale(LC_CTYPE, old_locale));
#endif
}

TEST(AsciiToFoo, All) {
#ifndef __ANDROID__
  // temporarily change locale to C. It should already be C, but just for safety
  const char* old_locale = setlocale(LC_CTYPE, "C");
  ASSERT_TRUE(old_locale != nullptr);
#endif

  for (int i = 0; i < 256; i++) {
    const auto c = static_cast<unsigned char>(i);
    if (turbo::ascii_islower(c))
      EXPECT_EQ(turbo::ascii_toupper(c), 'A' + (i - 'a')) << c;
    else
      EXPECT_EQ(turbo::ascii_toupper(c), static_cast<char>(i)) << c;

    if (turbo::ascii_isupper(c))
      EXPECT_EQ(turbo::ascii_tolower(c), 'a' + (i - 'A')) << c;
    else
      EXPECT_EQ(turbo::ascii_tolower(c), static_cast<char>(i)) << c;

    // These CHECKs only hold in a C locale.
    EXPECT_EQ(static_cast<char>(tolower(i)), turbo::ascii_tolower(c)) << c;
    EXPECT_EQ(static_cast<char>(toupper(i)), turbo::ascii_toupper(c)) << c;
  }
#ifndef __ANDROID__
  // restore the old locale.
  ASSERT_TRUE(setlocale(LC_CTYPE, old_locale));
#endif
}

TEST(AsciiStrTo, Lower) {
  const char buf[] = "ABCDEF";
  const std::string str("GHIJKL");
  const std::string str2("MNOPQR");
  const turbo::string_view sp(str2);
  const std::string long_str("ABCDEFGHIJKLMNOPQRSTUVWXYZ1!a");
  std::string mutable_str("_`?@[{AMNOPQRSTUVWXYZ");

  EXPECT_EQ("abcdef", turbo::str_to_lower(buf));
  EXPECT_EQ("ghijkl", turbo::str_to_lower(str));
  EXPECT_EQ("mnopqr", turbo::str_to_lower(sp));
  EXPECT_EQ("abcdefghijklmnopqrstuvwxyz1!a", turbo::str_to_lower(long_str));

  turbo::str_to_lower(&mutable_str);
  EXPECT_EQ("_`?@[{amnopqrstuvwxyz", mutable_str);

  char mutable_buf[] = "Mutable";
  std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                 mutable_buf, turbo::ascii_tolower);
  EXPECT_STREQ("mutable", mutable_buf);
}

TEST(AsciiStrTo, Upper) {
  const char buf[] = "abcdef";
  const std::string str("ghijkl");
  const std::string str2("_`?@[{amnopqrstuvwxyz");
  const turbo::string_view sp(str2);
  const std::string long_str("abcdefghijklmnopqrstuvwxyz1!A");

  EXPECT_EQ("ABCDEF", turbo::str_to_upper(buf));
  EXPECT_EQ("GHIJKL", turbo::str_to_upper(str));
  EXPECT_EQ("_`?@[{AMNOPQRSTUVWXYZ", turbo::str_to_upper(sp));
  EXPECT_EQ("ABCDEFGHIJKLMNOPQRSTUVWXYZ1!A", turbo::str_to_upper(long_str));

  char mutable_buf[] = "Mutable";
  std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                 mutable_buf, turbo::ascii_toupper);
  EXPECT_STREQ("MUTABLE", mutable_buf);
}

TEST(trim_left, FromStringView) {
  EXPECT_EQ(turbo::string_view{},
            turbo::trim_left(turbo::string_view{}));
  EXPECT_EQ("foo", turbo::trim_left({"foo"}));
  EXPECT_EQ("foo", turbo::trim_left({"\t  \n\f\r\n\vfoo"}));
  EXPECT_EQ("foo foo\n ",
            turbo::trim_left({"\t  \n\f\r\n\vfoo foo\n "}));
  EXPECT_EQ(turbo::string_view{}, turbo::trim_left(
                                     {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_left, InPlace) {
  std::string str;

  turbo::trim_left(&str);
  EXPECT_EQ("", str);

  str = "foo";
  turbo::trim_left(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo";
  turbo::trim_left(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo foo\n ";
  turbo::trim_left(&str);
  EXPECT_EQ("foo foo\n ", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  turbo::trim_left(&str);
  EXPECT_EQ(turbo::string_view{}, str);
}

TEST(trim_right, FromStringView) {
  EXPECT_EQ(turbo::string_view{},
            turbo::trim_right(turbo::string_view{}));
  EXPECT_EQ("foo", turbo::trim_right({"foo"}));
  EXPECT_EQ("foo", turbo::trim_right({"foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(" \nfoo foo",
            turbo::trim_right({" \nfoo foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(turbo::string_view{}, turbo::trim_right(
                                     {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_right, InPlace) {
  std::string str;

  turbo::trim_right(&str);
  EXPECT_EQ("", str);

  str = "foo";
  turbo::trim_right(&str);
  EXPECT_EQ("foo", str);

  str = "foo\t  \n\f\r\n\v";
  turbo::trim_right(&str);
  EXPECT_EQ("foo", str);

  str = " \nfoo foo\t  \n\f\r\n\v";
  turbo::trim_right(&str);
  EXPECT_EQ(" \nfoo foo", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  turbo::trim_right(&str);
  EXPECT_EQ(turbo::string_view{}, str);
}

TEST(trim_all, FromStringView) {
  EXPECT_EQ(turbo::string_view{},
            turbo::trim_all(turbo::string_view{}));
  EXPECT_EQ("foo", turbo::trim_all({"foo"}));
  EXPECT_EQ("foo",
            turbo::trim_all({"\t  \n\f\r\n\vfoo\t  \n\f\r\n\v"}));
  EXPECT_EQ("foo foo", turbo::trim_all(
                           {"\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(turbo::string_view{},
            turbo::trim_all({"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_all, InPlace) {
  std::string str;

  turbo::trim_all(&str);
  EXPECT_EQ("", str);

  str = "foo";
  turbo::trim_all(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo\t  \n\f\r\n\v";
  turbo::trim_all(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v";
  turbo::trim_all(&str);
  EXPECT_EQ("foo foo", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  turbo::trim_all(&str);
  EXPECT_EQ(turbo::string_view{}, str);
}

TEST(trim_complete, InPlace) {
  const char* inputs[] = {"No extra space",
                          "  Leading whitespace",
                          "Trailing whitespace  ",
                          "  Leading and trailing  ",
                          " Whitespace \t  in\v   middle  ",
                          "'Eeeeep!  \n Newlines!\n",
                          "nospaces",
                          "",
                          "\n\t a\t\n\nb \t\n"};

  const char* outputs[] = {
      "No extra space",
      "Leading whitespace",
      "Trailing whitespace",
      "Leading and trailing",
      "Whitespace in middle",
      "'Eeeeep! Newlines!",
      "nospaces",
      "",
      "a\nb",
  };
  const int NUM_TESTS = TURBO_ARRAYSIZE(inputs);

  for (int i = 0; i < NUM_TESTS; i++) {
    std::string s(inputs[i]);
    turbo::trim_complete(&s);
    EXPECT_EQ(outputs[i], s);
  }
}

}  // namespace
