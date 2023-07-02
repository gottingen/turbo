//
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
#ifndef TURBO_STRINGS_STR_CASE_CONV_H_
#define TURBO_STRINGS_STR_CASE_CONV_H_
#include "turbo/platform/port.h"
#include "turbo/meta/type_traits.h"

namespace turbo {
    TURBO_NAMESPACE_BEGIN


    // Converts the characters in `s` to lowercase, changing the contents of `s`.
    template<typename String>
    TURBO_MUST_USE_RESULT typename std::enable_if<turbo::is_string_type<String>::value>::type
    StrToLower(String *s);


    // Creates a lowercase string from a given std::string_view.

    template<typename String = std::string>
    TURBO_MUST_USE_RESULT inline typename std::enable_if<turbo::is_string_type<String>::value, String>::type
    StrToLower(std::string_view s) {
        String result(s);
        turbo::StrToLower(&result);
        return result;
    }


    // Converts the characters in `s` to uppercase, changing the contents of `s`.
    template<typename String>
    TURBO_MUST_USE_RESULT typename std::enable_if<turbo::is_string_type<String>::value>::type
    StrToUpper(String *s);

    // Creates an uppercase string from a given std::string_view.
    template<typename String = std::string>
    TURBO_MUST_USE_RESULT inline typename std::enable_if<turbo::is_string_type<String>::value, String>::type
    StrToUpper(std::string_view s) {
        String result(s);
        turbo::StrToUpper(&result);
        return result;
    }


    TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_STR_CASE_CONV_H_
