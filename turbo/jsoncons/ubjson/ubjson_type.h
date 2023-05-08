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

#ifndef JSONCONS_UBJSON_UBJSON_TYPE_HPP
#define JSONCONS_UBJSON_UBJSON_TYPE_HPP

#include <string>
#include <memory>
#include "turbo/jsoncons/config/jsoncons_config.h"

namespace turbo { namespace ubjson {

    namespace ubjson_type
    {
        const uint8_t null_type = 'Z';
        const uint8_t no_op_type = 'N';
        const uint8_t true_type = 'T';
        const uint8_t false_type = 'F';
        const uint8_t int8_type = 'i';
        const uint8_t uint8_type = 'U';
        const uint8_t int16_type = 'I';
        const uint8_t int32_type = 'l';
        const uint8_t int64_type = 'L';
        const uint8_t float32_type = 'd';
        const uint8_t float64_type = 'D';
        const uint8_t high_precision_number_type = 'H';
        const uint8_t char_type = 'C';
        const uint8_t string_type = 'S';
        const uint8_t start_array_marker = '[';
        const uint8_t end_array_marker = ']';
        const uint8_t start_object_marker = '{';
        const uint8_t end_object_marker = '}';
        const uint8_t type_marker = '$';
        const uint8_t count_marker = '#';
    }
 
} // namespace ubjson
} // namespace turbo

#endif
