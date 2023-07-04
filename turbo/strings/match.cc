// Copyright 2020 The Turbo Authors.
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

#include "turbo/strings/match.h"
#include "turbo/strings/str_case_conv.h"
#include "turbo/strings/internal/memutil.h"

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    bool StrIgnoreCaseContains(std::string_view haystack,
                           std::string_view needle) noexcept {
        auto ih = StrToLower(haystack);
        auto in = StrToLower(needle);
        return StrContains(ih, in);
    }

    bool StrIgnoreCaseContains(std::string_view haystack, char needle) noexcept {
        auto lc = turbo::ascii_tolower(needle);
        auto uc = turbo::ascii_toupper(needle);
        for (auto c: haystack) {
            if (c == lc || c == uc) {
                return true;
            }
        }
        return false;
    }

    bool EqualsIgnoreCase(std::string_view piece1,
                          std::string_view piece2) noexcept {
        return (piece1.size() == piece2.size() &&
                0 == turbo::strings_internal::memcasecmp(piece1.data(), piece2.data(),
                                                         piece1.size()));
        // memcasecmp uses turbo::ascii_tolower().
    }

    bool StartsWithIgnoreCase(std::string_view text,
                              std::string_view prefix) noexcept {
        return (text.size() >= prefix.size()) &&
               EqualsIgnoreCase(text.substr(0, prefix.size()), prefix);
    }

    bool EndsWithIgnoreCase(std::string_view text,
                            std::string_view suffix) noexcept {
        return (text.size() >= suffix.size()) &&
               EqualsIgnoreCase(text.substr(text.size() - suffix.size()), suffix);
    }

    TURBO_NAMESPACE_END
}  // namespace turbo
