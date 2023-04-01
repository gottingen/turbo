// Copyright 2023 The Turbo Authors.
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


/**
 * Big-Endian/Little-endian tests.
 */
#include "turbo/unicode/utf.h"
#include <turbo/unicode/test/helpers/test.h>

// We use explicit arrays so that no funny business is possible.

//
// s = "@\u00A7\u2208\U0001D4AA"
const unsigned char utf8_string[] = {0x40,0xc2,0xa7,0xe2,0x88,0x88,0xf0,0x9d,0x92,0xaa};
const char *utf8 = reinterpret_cast<const char*>(utf8_string);
const size_t utf8_size = sizeof(utf8_string)/sizeof(uint8_t);

const unsigned char utf16le_string[] = {0x40,0x00,0xa7,0x00,0x08,0x22,0x35,0xd8,0xaa,0xdc};
const char16_t *utf16le = reinterpret_cast<const char16_t*>(utf16le_string);
const size_t utf16_size = sizeof(utf16le_string)/sizeof(uint16_t);


const unsigned char utf16be_string[] = {0x00,0x40,0x00,0xa7,0x22,0x08,0xd8,0x35,0xdc,0xaa};
const char16_t *utf16be = reinterpret_cast<const char16_t*>(utf16be_string);
#if TURBO_IS_BIG_ENDIAN
const char16_t *utf16 = utf16be;
#else
const char16_t *utf16 = utf16le;
#endif

// Native order
#if TURBO_IS_BIG_ENDIAN
const unsigned char utf32_string[] = {0x00,0x00,0x00,0x40,0x00,0x00,0x00,0xa7,0x00,0x00,0x22,0x08,0x00,0x01,0xd4,0xaa};
const char32_t *utf32 = reinterpret_cast<const char32_t*>(utf32_string); // Technically undefined behavior.
#else
const unsigned char utf32_string[] = {0x40,0x00,0x00,0x00,0xa7,0x00,0x00,0x00,0x08,0x22,0x00,0x00,0xaa,0xd4,0x01,0x00};
const char32_t *utf32 = reinterpret_cast<const char32_t*>(utf32_string); // Technically undefined behavior.
#endif
const size_t utf32_size = sizeof(utf32_string)/sizeof(char32_t);
const size_t number_of_code_points = utf32_size;


TEST(ValidateUtf8) {
    turbo::result res = implementation.ValidateUtf8WithErrors(utf8, utf8_size);
    ASSERT_EQUAL(res.error, turbo::error_code::SUCCESS);
}

TEST(ValidateUtf16Le) {
    turbo::result res = implementation.ValidateUtf16LeWithErrors(utf16le, utf16_size);
    ASSERT_EQUAL(res.error, turbo::error_code::SUCCESS);
}

TEST(ValidateUtf16Be) {
    turbo::result res = implementation.ValidateUtf16BeWithErrors(utf16be, utf16_size);
    ASSERT_EQUAL(res.error, turbo::error_code::SUCCESS);
}

TEST(ValidateUtf32) {
    turbo::result res = implementation.ValidateUtf32WithErrors(utf32, utf32_size);
    ASSERT_EQUAL(res.error, turbo::error_code::SUCCESS);
}

TEST(CountUtf8) {
    size_t count = implementation.CountUtf8(utf8, utf8_size);
    ASSERT_EQUAL(count, number_of_code_points);
}

TEST(CountUtf16Le) {
    size_t count = implementation.CountUtf16Le(utf16le, utf16_size);
    ASSERT_EQUAL(count, number_of_code_points);
}

TEST(CountUtf16Be) {
    size_t count = implementation.CountUtf16Be(reinterpret_cast<const char16_t*>(utf16be_string), sizeof(utf16be_string)/sizeof(uint16_t));
    ASSERT_EQUAL(count, number_of_code_points);
}

TEST(ConvertUtf8ToUtf16Le) {
    char16_t buffer[utf16_size];
    size_t count = implementation.ConvertUtf8ToUtf16Le(utf8, utf8_size, buffer);
    ASSERT_EQUAL(count, utf16_size);
    for(size_t i = 0; i < utf16_size; i++) {
        ASSERT_EQUAL(buffer[i], utf16le[i]);
    }
}

TEST(ConvertUtf8ToUtf16Be) {
    char16_t buffer[utf16_size];
    size_t count = implementation.ConvertUtf8ToUtf16Be(utf8, utf8_size, buffer);
    ASSERT_EQUAL(count, utf16_size);
    for(size_t i = 0; i < utf16_size; i++) {
        ASSERT_EQUAL(buffer[i], utf16be[i]);
    }
}


TEST(ConvertUtf8ToUtf32) {
    char32_t buffer[utf32_size];
    size_t count = implementation.ConvertUtf8ToUtf32(utf8, utf8_size, buffer);
    ASSERT_EQUAL(count, utf32_size);
    for(size_t i = 0; i < utf32_size; i++) {
        ASSERT_EQUAL(buffer[i], utf32[i]);
    }
}

TEST(ConvertUtf32ToUtf8) {
    char buffer[utf8_size];
    size_t count = implementation.ConvertUtf32ToUtf8(utf32, utf32_size, buffer);
    ASSERT_EQUAL(count, utf8_size);
    for(size_t i = 0; i < utf8_size; i++) {
        ASSERT_EQUAL(buffer[i], utf8[i]);
    }
}

TEST(ConvertUtf32ToUtf16Be) {
    char buffer[utf8_size];
    size_t count = implementation.ConvertUtf32ToUtf8(utf32, utf32_size, buffer);
    ASSERT_EQUAL(count, utf8_size);
    for(size_t i = 0; i < utf8_size; i++) {
        ASSERT_EQUAL(buffer[i], utf8[i]);
    }
}

TEST(ConvertUtf32ToUtf16Le) {
    char buffer[utf8_size];
    size_t count = implementation.ConvertUtf32ToUtf8(utf32, utf32_size, buffer);
    ASSERT_EQUAL(count, utf8_size);
    for(size_t i = 0; i < utf8_size; i++) {
        ASSERT_EQUAL(buffer[i], utf8[i]);
    }
}


TEST(ConvertUtf16LeToUtf8) {
    char buffer[utf8_size];
    size_t count = implementation.ConvertUtf16LeToUtf8(utf16le, utf16_size, buffer);
    ASSERT_EQUAL(count, utf8_size);
    for(size_t i = 0; i < utf8_size; i++) {
        ASSERT_EQUAL(buffer[i], utf8[i]);
    }
}


TEST(ConvertUtf16LeToUtf32) {
    char32_t buffer[utf32_size];
    size_t count = implementation.ConvertUtf16LeToUtf32(utf16le, utf16_size, buffer);
    ASSERT_EQUAL(count, utf32_size);
    for(size_t i = 0; i < utf32_size; i++) {
        ASSERT_EQUAL(buffer[i], utf32[i]);
    }
}

TEST(ConvertUtf16BeToUtf8) {
    char buffer[utf8_size];
    size_t count = implementation.ConvertUtf16BeToUtf8(utf16be, utf16_size, buffer);
    ASSERT_EQUAL(count, utf8_size);
    for(size_t i = 0; i < utf8_size; i++) {
        ASSERT_EQUAL(buffer[i], utf8[i]);
    }
}


TEST(ConvertUtf16BeToUtf32) {
    char32_t buffer[utf32_size];
    size_t count = implementation.ConvertUtf16BeToUtf32(utf16be, utf16_size, buffer);
    ASSERT_EQUAL(count, utf32_size);
    for(size_t i = 0; i < utf32_size; i++) {
        ASSERT_EQUAL(buffer[i], utf32[i]);
    }
}

int main(int argc, char* argv[]) {
  return turbo::test::main(argc, argv);
}