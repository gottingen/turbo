//
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
//
// Created by jeff on 24-6-6.
//

#include <turbo/hash/hash.h>
#include <turbo/hash/internal/murmur3.h>

namespace turbo {

    uint32_t hash32(const void *data, size_t size, uint32_t seed) {
        uint32_t hash;
        hash_internal::MurmurHash3_x86_32(data, size, seed, &hash);
        return hash;
    }

    void hash128(const void *data, size_t size, std::array<uint64_t, 2> *result, uint32_t seed) {
        hash_internal::MurmurHash3_x64_128(data, size, seed, result->data());
    }
    void hash128(const void *data, size_t size, std::array<uint32_t , 4> *result, uint32_t seed) {
        hash_internal::MurmurHash3_x86_128(data, size, seed, result->data());
    }
}