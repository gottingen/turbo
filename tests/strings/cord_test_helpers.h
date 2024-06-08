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

#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include <turbo/base/config.h>
#include <turbo/strings/cord.h>
#include <turbo/strings/internal/cord_internal.h>
#include <turbo/strings/string_view.h>

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    enum class TestCordSize {
        // An empty value
        kEmpty = 0,

        // An inlined string value
        kInlined = cord_internal::kMaxInline / 2 + 1,

        // 'Well known' SSO lengths (excluding terminating zero).
        // libstdcxx has a maximum SSO of 15, libc++ has a maximum SSO of 22.
        kStringSso1 = 15,
        kStringSso2 = 22,

        // A string value which is too large to fit in inlined data, but small enough
        // such that Cord prefers copying the value if possible, i.e.: not stealing
        // std::string inputs, or referencing existing CordReps on append, etc.
        kSmall = cord_internal::kMaxBytesToCopy / 2 + 1,

        // A string value large enough that Cord prefers to reference or steal from
        // existing inputs rather than copying contents of the input.
        kMedium = cord_internal::kMaxFlatLength / 2 + 1,

        // A string value large enough to cause it to be stored in multiple flats.
        kLarge = cord_internal::kMaxFlatLength * 4
    };

    // To string helper
    inline std::string_view ToString(TestCordSize size) {
        switch (size) {
            case TestCordSize::kEmpty:
                return "Empty";
            case TestCordSize::kInlined:
                return "Inlined";
            case TestCordSize::kSmall:
                return "Small";
            case TestCordSize::kStringSso1:
                return "StringSso1";
            case TestCordSize::kStringSso2:
                return "StringSso2";
            case TestCordSize::kMedium:
                return "Medium";
            case TestCordSize::kLarge:
                return "Large";
        }
        return "???";
    }

    // Returns the length matching the specified size
    inline size_t Length(TestCordSize size) { return static_cast<size_t>(size); }

    // Stream output helper
    inline std::ostream &operator<<(std::ostream &stream, TestCordSize size) {
        return stream << ToString(size);
    }

    // Creates a multi-segment Cord from an iterable container of strings.  The
    // resulting Cord is guaranteed to have one segment for every string in the
    // container.  This allows code to be unit tested with multi-segment Cord
    // inputs.
    //
    // Example:
    //
    //   turbo::Cord c = turbo::MakeFragmentedCord({"A ", "fragmented ", "Cord"});
    //   EXPECT_FALSE(c.GetFlat(&unused));
    //
    // The mechanism by which this Cord is created is an implementation detail.  Any
    // implementation that produces a multi-segment Cord may produce a flat Cord in
    // the future as new optimizations are added to the Cord class.
    // MakeFragmentedCord will, however, always be updated to return a multi-segment
    // Cord.
    template<typename Container>
    Cord MakeFragmentedCord(const Container &c) {
        Cord result;
        for (const auto &s: c) {
            auto *external = new std::string(s);
            Cord tmp = turbo::make_cord_from_external(
                    *external, [external](std::string_view) { delete external; });
            tmp.prepend(result);
            result = tmp;
        }
        return result;
    }

    inline Cord MakeFragmentedCord(std::initializer_list<std::string_view> list) {
        return MakeFragmentedCord<std::initializer_list<std::string_view>>(list);
    }

}  // namespace turbo

