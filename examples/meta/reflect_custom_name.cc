// Copyright 2023 The titan-search Authors.
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

#include <iostream>

#include "turbo/meta/reflect.h"

enum class Color {
    RED = -10, BLUE = 0, GREEN = 10
};
enum class Numbers {
    One, Two, Three
};

#if defined(REFLECT_ENUM_SUPPORTED)

// Сustom definitions of names for enum.
// Specialization of `enum_name` must be injected in `namespace turbo::customize`.
template<>
constexpr std::string_view turbo::customize::enum_name<Color>(Color value) noexcept {
    switch (value) {
        case Color::RED:
            return "the red color";
        case Color::BLUE:
            return "The BLUE";
        case Color::GREEN:
            return {}; // Empty string for default value.
    }
    return {}; // Empty string for unknown value.
}

// Сustom definitions of names for enum.
// Specialization of `enum_name` must be injected in `namespace turbo::customize`.
template<>
constexpr std::string_view turbo::customize::enum_name<Numbers>(Numbers value) noexcept {
    switch (value) {
        case Numbers::One:
            return "the one";
        default:
            return {}; // Empty string for default or unknown value.
    }
}

#endif

// Сustom definitions of names for type.
// Specialization of `type_name` must be injected in `namespace turbo::customize`.
template<>
constexpr std::string_view turbo::customize::type_name<Color>() noexcept {
    return "The Color";
}

class a1_test {
};

class a2_test {
};

// Сustom definitions of names for type.
// Specialization of `type_name` must be injected in `namespace turbo::customize`.
template<>
constexpr std::string_view turbo::customize::type_name<a1_test>() noexcept {
    return "Animal";
}

int main() {
#if defined(REFLECT_ENUM_SUPPORTED)
    std::cout << turbo::nameof_enum(Color::RED) << std::endl; // 'the red color'
    std::cout << turbo::nameof_enum(Color::BLUE) << std::endl; // 'The BLUE'
    std::cout << turbo::nameof_enum(Color::GREEN) << std::endl; // 'GREEN'

    std::cout << turbo::nameof_enum(Numbers::One) << std::endl; // 'the one'
    std::cout << turbo::nameof_enum(Numbers::Two) << std::endl; // 'Two'
    std::cout << turbo::nameof_enum(Numbers::Three) << std::endl; // 'Three'
#endif

    std::cout << turbo::nameof_type<Color>() << std::endl; // 'The Color'
    std::cout << turbo::nameof_type<Numbers>() << std::endl; // 'Numbers'
    std::cout << turbo::nameof_type<a1_test>() << std::endl; // 'Animal'
    std::cout << turbo::nameof_type<a2_test>() << std::endl; // 'a2_test'

    return 0;
}
