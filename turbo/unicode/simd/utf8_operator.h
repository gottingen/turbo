// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//


#ifndef TURBO_UNICODE_SIMD_UTF8_OPERATOR_H_
#define TURBO_UNICODE_SIMD_UTF8_OPERATOR_H_

#include "turbo/simd/simd.h"
#include "turbo/meta/type_traits.h"
#include "turbo/unicode/scalar/validate.h"
#include "turbo/base/bits.h"

namespace turbo::unicode {

    struct CountCodePoints {
        template <class Tag, class Arch>
        size_t operator()(Arch, const char* in,  size_t size,Tag) {
            using b_type = turbo::simd::batch<char, Arch>;
            std::size_t inc = b_type::size;
            std::size_t vec_size = size - size % inc;
            size_t count = 0;
            for (std::size_t i = 0; i < vec_size; i += inc) {
                b_type avec = b_type::load(&in[i], Tag());
                auto mask_bool = avec < (-65 + 1);
                auto mask = mask_bool.mask();
                count += turbo::popcount(mask);
            }
            // Remaining part that cannot be vectorize
            return count + utf8::count_code_points(in + vec_size, size - vec_size);
        }
    };

    struct Utf16LengthFromUtf8 {
        template <class Tag, class Arch>
        size_t operator()(Arch, const char* in,  size_t size,Tag) {
            using b_type = turbo::simd::batch<char, Arch>;
            std::size_t inc = b_type::size;
            std::size_t vec_size = size - size % inc;
            size_t count = 0;
            static const turbo::simd::batch<char, Arch> mask240 = turbo::simd::broadcast(-16);
            for (std::size_t i = 0; i < vec_size; i += inc) {
                b_type avec = b_type::load(&in[i], Tag());
                auto mask_bool = avec < (-65 + 1);
                count += 64 - turbo::popcount(mask_bool.mask());
                auto utf8_4byte_mask = avec >= mask240;
                count += turbo::popcount(utf8_4byte_mask.mask());
            }
            // Remaining part that cannot be vectorize
            return count + utf8::count_code_points(in + vec_size, size - vec_size);
        }
    };

}  // namespace turbo::unicode
#endif  // TURBO_UNICODE_SIMD_UTF8_OPERATOR_H_
