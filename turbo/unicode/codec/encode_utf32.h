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

#include <cstdint>

namespace turbo::unicode::utf32 {

    // returns whether the value can be represented in the UTF-32
    bool valid_value(uint32_t value) {

        if (value > 0x10FFFF)
            return false;

        if ((value >= 0xD800) && (value <= 0xDFFF))
            return false;

        return true;
    }

    // Encodes the value in UTF-32
    // Returns 1 if the value can be encoded
    // Returns 0 if the value cannot be encoded
    template<typename CONSUMER>
    int encode(uint32_t value, CONSUMER consumer) {
        if (!valid_value(value))
            return 0;

        consumer(value);
        return 1;
    }

} // namespace turbo::unicode::utf32