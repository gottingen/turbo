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

#pragma once

#include <cstdio>

namespace turbo { namespace tests { namespace reference { namespace utf32 {

    enum class Error {
        too_large,
        forbidden_range
    };

    template <typename CONSUMER, typename ERROR_HANDLER>
    bool decode(const char32_t* codepoints, size_t size, CONSUMER consumer, ERROR_HANDLER error_handler) {
        const char32_t* curr = codepoints;
        const char32_t* end = codepoints + size;

        while (curr != end) {
            const uint32_t word = *curr;

            if (word > 0x10FFFF) {
                if (!error_handler(codepoints, curr, Error::too_large))
                    return false;

                continue;
            }

            if (word >= 0xD800 && word <= 0xDFFF) { // required the next word, but we're already at the end of data
                if (!error_handler(codepoints, curr, Error::forbidden_range))
                    return false;

                break;
            }

            consumer(word);

            curr ++;
        }

        return true;
    }

}}}}
