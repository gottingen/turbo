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

/* Returns:
   - pointer to the last unprocessed character (a scalar fallback should check the rest);
   - nullptr if an error was detected.
*/
const char32_t* avx2_validate_utf32le(const char32_t* input, size_t size) {
    const char32_t* end = input + size;

    const __m256i standardmax = _mm256_set1_epi32(0x10ffff);
    const __m256i offset = _mm256_set1_epi32(0xffff2000);
    const __m256i standardoffsetmax = _mm256_set1_epi32(0xfffff7ff);
    __m256i currentmax = _mm256_setzero_si256();
    __m256i currentoffsetmax = _mm256_setzero_si256();

    while (input + 8 < end) {
        const __m256i in = _mm256_loadu_si256((__m256i *)const_cast<char32_t*>(input));
        currentmax = _mm256_max_epu32(in,currentmax);
        currentoffsetmax = _mm256_max_epu32(_mm256_add_epi32(in, offset), currentoffsetmax);
        input += 8;
    }
    __m256i is_zero = _mm256_xor_si256(_mm256_max_epu32(currentmax, standardmax), standardmax);
    if(_mm256_testz_si256(is_zero, is_zero) == 0) {
        return nullptr;
    }

    is_zero = _mm256_xor_si256(_mm256_max_epu32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
    if(_mm256_testz_si256(is_zero, is_zero) == 0) {
        return nullptr;
    }

    return input;
}


const result avx2_validate_utf32le_with_errors(const char32_t* input, size_t size) {
    const char32_t* start = input;
    const char32_t* end = input + size;

    const __m256i standardmax = _mm256_set1_epi32(0x10ffff);
    const __m256i offset = _mm256_set1_epi32(0xffff2000);
    const __m256i standardoffsetmax = _mm256_set1_epi32(0xfffff7ff);
    __m256i currentmax = _mm256_setzero_si256();
    __m256i currentoffsetmax = _mm256_setzero_si256();

    while (input + 8 < end) {
        const __m256i in = _mm256_loadu_si256((__m256i *)const_cast<char32_t*>(input));
        currentmax = _mm256_max_epu32(in,currentmax);
        currentoffsetmax = _mm256_max_epu32(_mm256_add_epi32(in, offset), currentoffsetmax);

        __m256i is_zero = _mm256_xor_si256(_mm256_max_epu32(currentmax, standardmax), standardmax);
        if(_mm256_testz_si256(is_zero, is_zero) == 0) {
            return result(error_code::TOO_LARGE, input - start);
        }

        is_zero = _mm256_xor_si256(_mm256_max_epu32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
        if(_mm256_testz_si256(is_zero, is_zero) == 0) {
            return result(error_code::SURROGATE, input - start);
        }
        input += 8;
    }

    return result(error_code::SUCCESS, input - start);
}