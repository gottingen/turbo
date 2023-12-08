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


#ifndef TURBO_UTF_CONVERTER_H_
#define TURBO_UTF_CONVERTER_H_

#include "turbo/utf/fwd.h"
#include "turbo/utf/scalar/utf_converter.h"
#include "turbo/utf/avx2/utf_converter.h"
#include "turbo/meta/type_traits.h"
#include "turbo/utf/utf_engine.h"

namespace turbo  {

    template<typename Engine = UtfScalarEngine, check_requires<is_utf_engine<Engine>>>
    inline turbo::EncodingType auto_detect_encoding(const char * input, size_t length) noexcept {
        return UtfConverter<Engine>::get_instance()->auto_detect_encoding(input, length);
    }

    template<typename Engine = UtfScalarEngine, check_requires<is_utf_engine<Engine>>>
    inline turbo::EncodingType auto_detect_encoding(const uint8_t * input, size_t length) noexcept {
        return auto_detect_encoding<Engine>(reinterpret_cast<const char *>(input), length);
    }

    template<typename Engine = UtfScalarEngine, check_requires<is_utf_engine<Engine>>>
    inline turbo::EncodingType auto_detect_encoding(const std::string_view &str) noexcept {
        return auto_detect_encoding<Engine>(str.data(), str.size());
    }

}  // namespace turbo

#endif // TURBO_UTF_CONVERTER_H_
