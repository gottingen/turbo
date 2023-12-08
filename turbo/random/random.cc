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
//
#include "turbo/random/random.h"

namespace turbo {

    void fast_random_bytes(void* output, size_t output_length) {
        const size_t n = output_length / 8;
        for (size_t i = 0; i < n; ++i) {
            static_cast<uint64_t*>(output)[i] = fast_random();
        }
        const size_t m = output_length - n * 8;
        if (m) {
            uint8_t* p = static_cast<uint8_t*>(output) + n * 8;
            uint64_t r = fast_random();
            for (size_t i = 0; i < m; ++i) {
                p[i] = (r & 0xFF);
                r = (r >> 8);
            }
        }
    }

    std::string fast_random_printable(size_t length) {
        std::string result(length, 0);
        const size_t halflen = length/2;
        fast_random_bytes(&result[0], halflen);
        for (size_t i = 0; i < halflen; ++i) {
            const uint8_t b = result[halflen - 1 - i];
            result[length - 1 - 2*i] = 'A' + (b & 0xF);
            result[length - 2 - 2*i] = 'A' + (b >> 4);
        }
        if (halflen * 2 != length) {
            result[0] = 'A' + (fast_random() % 16);
        }
        return result;
    }
}