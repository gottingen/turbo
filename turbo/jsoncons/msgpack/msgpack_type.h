// Copyright 2013-2023 Daniel Parker
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
//

#ifndef JSONCONS_MSGPACK_MSGPACK_TYPE_HPP
#define JSONCONS_MSGPACK_MSGPACK_TYPE_HPP

#include <string>
#include <memory>
#include "turbo/jsoncons/config/jsoncons_config.h"

namespace turbo { namespace msgpack {

    namespace msgpack_type
    {
        const uint8_t positive_fixint_base_type = 0x00;
        const uint8_t nil_type = 0xc0;
        const uint8_t false_type = 0xc2;
        const uint8_t true_type = 0xc3;
        const uint8_t float32_type = 0xca;
        const uint8_t float64_type = 0xcb;
        const uint8_t uint8_type = 0xcc;
        const uint8_t uint16_type = 0xcd;
        const uint8_t uint32_type = 0xce;
        const uint8_t uint64_type = 0xcf;
        const uint8_t int8_type = 0xd0;
        const uint8_t int16_type = 0xd1;
        const uint8_t int32_type = 0xd2;
        const uint8_t int64_type = 0xd3;

        const uint8_t fixmap_base_type = 0x80;
        const uint8_t fixarray_base_type = 0x90;
        const uint8_t fixstr_base_type = 0xa0;
        const uint8_t str8_type = 0xd9;
        const uint8_t str16_type = 0xda;
        const uint8_t str32_type = 0xdb;

        const uint8_t bin8_type = 0xc4; //  0xC4
        const uint8_t bin16_type = 0xc5;
        const uint8_t bin32_type = 0xc6;

        const uint8_t fixext1_type = 0xd4;
        const uint8_t fixext2_type = 0xd5;
        const uint8_t fixext4_type = 0xd6;
        const uint8_t fixext8_type = 0xd7;
        const uint8_t fixext16_type = 0xd8;
        const uint8_t ext8_type = 0xc7; //  0xC4
        const uint8_t ext16_type = 0xc8;
        const uint8_t ext32_type = 0xc9;

        const uint8_t array16_type = 0xdc;
        const uint8_t array32_type = 0xdd;
        const uint8_t map16_type = 0xde;
        const uint8_t map32_type = 0xdf;
        const uint8_t negative_fixint_base_type = 0xe0;
    }
 
} // namespace msgpack
} // namespace turbo

#endif
