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

#include <string>

namespace turbo {

enum encoding_type {
        UTF8 = 1,       // BOM 0xef 0xbb 0xbf
        UTF16_LE = 2,   // BOM 0xff 0xfe
        UTF16_BE = 4,   // BOM 0xfe 0xff
        UTF32_LE = 8,   // BOM 0xff 0xfe 0x00 0x00
        UTF32_BE = 16,   // BOM 0x00 0x00 0xfe 0xff

        unspecified = 0
};

enum endianness {
        LITTLE,
        BIG
};

bool match_system(endianness e);

std::string to_string(encoding_type bom);

// Note that BOM for UTF8 is discouraged.
namespace BOM {

/**
 * Checks for a BOM. If not, returns unspecified
 * @param input         the string to process
 * @param length        the length of the string in words
 * @return the corresponding encoding
 */

encoding_type check_bom(const uint8_t* byte, size_t length);
encoding_type check_bom(const char* byte, size_t length);
/**
 * Returns the size, in bytes, of the BOM for a given encoding type.
 * Note that UTF8 BOM are discouraged.
 * @param bom         the encoding type
 * @return the size in bytes of the corresponding BOM
 */
size_t bom_byte_size(encoding_type bom);

} // BOM namespace
} // simdutf namespace
