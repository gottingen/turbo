// Copyright 2024 The Turbo Authors.
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

#include "turbo/strings/str_case_conv.h"
#include "turbo/strings/inlined_string.h"
#include "turbo/strings/ascii.h"

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value>::type
    StrToLower(String *s) {
        for (auto &ch: *s) {
            ch = turbo::ascii_tolower(static_cast<unsigned char>(ch));
        }
    }

    template<typename String>
    TURBO_MUST_USE_RESULT typename std::enable_if<turbo::is_string_type<String>::value>::type
    StrToUpper(String *s) {
        for (auto &ch: *s) {
            ch = turbo::ascii_toupper(static_cast<unsigned char>(ch));
        }
    }

    template void StrToLower(std::string *);

    template void StrToLower(turbo::inlined_string *);

    template void StrToUpper(std::string *);

    template void StrToUpper(turbo::inlined_string *);

    TURBO_NAMESPACE_END
}  // namespace turbo
