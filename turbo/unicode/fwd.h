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


#ifndef TURBO_UNICODE_FWD_H_
#define TURBO_UNICODE_FWD_H_

#include <type_traits>
#include "turbo/unicode/encoding_types.h"

namespace turbo::unicode {
    template<typename Engine>
    struct is_unicode_engine : std::false_type {
    };

    template<typename Engine>
    struct Converter {
        /**
         * @brief static function to get the instance of the engine
         * @return the instance of the engine
         */
        static Converter<Engine> *get_instance() {
            static Converter<Engine> instance;
            return &instance;
        }
    };

}  // namespace turbo::unicode

namespace turbo::unicode::simd {

    template<typename Engine, typename Child>
    struct base;
    template<typename Engine, typename T>
    struct simd8;

    template<typename Engine, typename T, typename Mask>
    struct base8;

    template<typename Engine, typename T>
    struct base8_numeric;

    template<typename Engine, typename T>
    struct simd8x64;

    template<typename Engine, typename T>
    struct simd16;

    template<typename Engine, typename T, typename Mask>
    struct base16;

    template<typename Engine, typename T>
    struct base16_numeric;

    template<typename Engine, typename T>
    struct simd16x32;


}  // namespace turbo::unicode::simd
#endif // TURBO_UNICODE_FWD_H_
