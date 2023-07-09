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

#include "turbo/strings/ascii.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>

#include "turbo/platform/port.h"
#include "turbo/strings/inlined_string.h"

namespace {

    TEST_CASE("AsciiIsFoo, All") {
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
                CHECK(turbo::ascii_isalpha(c));
            else
                CHECK(!turbo::ascii_isalpha(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if ((c >= '0' && c <= '9'))
                CHECK(turbo::ascii_isdigit(c));
            else
                CHECK(!turbo::ascii_isdigit(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (turbo::ascii_isalpha(c) || turbo::ascii_isdigit(c))
                CHECK(turbo::ascii_isalnum(c));
            else
                CHECK(!turbo::ascii_isalnum(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (i != '\0' && strchr(" \r\n\t\v\f", i))
                CHECK(turbo::ascii_isspace(c));
            else
                CHECK(!turbo::ascii_isspace(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (i >= 32 && i < 127)
                CHECK(turbo::ascii_isprint(c));
            else
                CHECK(!turbo::ascii_isprint(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (turbo::ascii_isprint(c) && !turbo::ascii_isspace(c) &&
                !turbo::ascii_isalnum(c)) {
                CHECK(turbo::ascii_ispunct(c));
            } else {
                CHECK(!turbo::ascii_ispunct(c));
            }
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (i == ' ' || i == '\t')
                CHECK(turbo::ascii_isblank(c));
            else
                CHECK(!turbo::ascii_isblank(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (i < 32 || i == 127)
                CHECK(turbo::ascii_iscntrl(c));
            else
                CHECK(!turbo::ascii_iscntrl(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (turbo::ascii_isdigit(c) || (i >= 'A' && i <= 'F') ||
                (i >= 'a' && i <= 'f')) {
                CHECK(turbo::ascii_isxdigit(c));
            } else {
                CHECK(!turbo::ascii_isxdigit(c));
            }
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (i > 32 && i < 127)
                CHECK(turbo::ascii_isgraph(c));
            else
                CHECK(!turbo::ascii_isgraph(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (i >= 'A' && i <= 'Z')
                CHECK(turbo::ascii_isupper(c));
            else
                CHECK(!turbo::ascii_isupper(c));
        }
        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (i >= 'a' && i <= 'z')
                CHECK(turbo::ascii_islower(c));
            else
                CHECK(!turbo::ascii_islower(c));
        }
        for (unsigned char c = 0; c < 128; c++) {
            CHECK(turbo::ascii_isascii(c));
        }
        for (int i = 128; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            CHECK(!turbo::ascii_isascii(c));
        }
    }

// Checks that turbo::ascii_isfoo returns the same value as isfoo in the C
// locale.
    TEST_CASE("AsciiIsFoo, SameAsIsFoo") {
#ifndef __ANDROID__
        // temporarily change locale to C. It should already be C, but just for safety
        const char *old_locale = setlocale(LC_CTYPE, "C");
        REQUIRE(old_locale != nullptr);
#endif

        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            CHECK_EQ(isalpha(c) != 0, turbo::ascii_isalpha(c));
            CHECK_EQ(isdigit(c) != 0, turbo::ascii_isdigit(c));
            CHECK_EQ(isalnum(c) != 0, turbo::ascii_isalnum(c));
            CHECK_EQ(isspace(c) != 0, turbo::ascii_isspace(c));
            CHECK_EQ(ispunct(c) != 0, turbo::ascii_ispunct(c));
            CHECK_EQ(isblank(c) != 0, turbo::ascii_isblank(c));
            CHECK_EQ(iscntrl(c) != 0, turbo::ascii_iscntrl(c));
            CHECK_EQ(isxdigit(c) != 0, turbo::ascii_isxdigit(c));
            CHECK_EQ(isprint(c) != 0, turbo::ascii_isprint(c));
            CHECK_EQ(isgraph(c) != 0, turbo::ascii_isgraph(c));
            CHECK_EQ(isupper(c) != 0, turbo::ascii_isupper(c));
            CHECK_EQ(islower(c) != 0, turbo::ascii_islower(c));
            CHECK_EQ(isascii(c) != 0, turbo::ascii_isascii(c));
        }

#ifndef __ANDROID__
        // restore the old locale.
        REQUIRE(setlocale(LC_CTYPE, old_locale));
#endif
    }

    TEST_CASE("AsciiToFoo, All") {

#ifndef __ANDROID__
        // temporarily change locale to C. It should already be C, but just for safety
        const char *old_locale = setlocale(LC_CTYPE, "C");
        REQUIRE(old_locale != nullptr);
#endif

        for (int i = 0; i < 256; i++) {
            const auto c = static_cast<unsigned char>(i);
            if (turbo::ascii_islower(c))
                CHECK_EQ(turbo::ascii_toupper(c), 'A' + (i - 'a'));
            else
                CHECK_EQ(turbo::ascii_toupper(c), static_cast<char>(i));

            if (turbo::ascii_isupper(c))
                CHECK_EQ(turbo::ascii_tolower(c), 'a' + (i - 'A'));
            else
                CHECK_EQ(turbo::ascii_tolower(c), static_cast<char>(i));

            // These CHECKs only hold in a C locale.
            CHECK_EQ(static_cast<char>(tolower(i)), turbo::ascii_tolower(c));
            CHECK_EQ(static_cast<char>(toupper(i)), turbo::ascii_toupper(c));
        }
#ifndef __ANDROID__
        // restore the old locale.
        REQUIRE(setlocale(LC_CTYPE, old_locale));
#endif
    }

}  // namespace
