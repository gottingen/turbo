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

#ifndef JSONCONS_BSON_BSON_TYPE_HPP
#define JSONCONS_BSON_BSON_TYPE_HPP

#include <string>
#include <memory>
#include "turbo/jsoncons/config/jsoncons_config.h"

namespace turbo { namespace bson {

    namespace bson_type
    {
        const uint8_t double_type = 0x01;
        const uint8_t string_type = 0x02; // UTF-8 string
        const uint8_t document_type = 0x03;
        const uint8_t array_type = 0x04;
        const uint8_t binary_type = 0x05;
        const uint8_t undefined_type = 0x06; // map to null
        const uint8_t object_id_type = 0x07;
        const uint8_t bool_type = 0x08;
        const uint8_t datetime_type = 0x09;
        const uint8_t null_type = 0x0a;
        const uint8_t regex_type = 0x0b;
        const uint8_t javascript_type = 0x0d;
        const uint8_t symbol_type = 0x0e; // deprecated, mapped to string
        const uint8_t javascript_with_scope_type = 0x0f; // unsupported
        const uint8_t int32_type = 0x10;
        const uint8_t timestamp_type = 0x11; // MongoDB internal Timestamp, uint64
        const uint8_t int64_type = 0x12;
        const uint8_t decimal128_type = 0x13;
        const uint8_t min_key_type = 0xff;
        const uint8_t max_key_type = 0x7f;
    }

    enum class bson_container_type {document, array};

}}

#endif
