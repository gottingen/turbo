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

#include "turbo/meta/reflect.h"

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

struct Base {
    virtual ~Base() = default;
};

struct Derived : Base {
};

struct SomeStruct {
    int somefield = 0;

    void SomeMethod1(int i) { somefield = i; }

    int SomeMethod2() const { return somefield; }
};

void SomeMethod3() {
    std::cout << REFLECT(SomeMethod3) << " no called!" << std::endl;
}

template<typename T, typename U>
std::string SomeMethod4(U value) {
    auto function_name = REFLECT(SomeMethod4<T, U>).str()
            .append("<")
            .append(REFLECT_TYPE(T))
            .append(", ")
            .append(REFLECT_TYPE(U))
            .append(">(")
            .append(REFLECT_TYPE(U))
            .append(" ")
            .append(REFLECT(value).data())
            .append(")");

    return function_name;
}

template<typename T>
class SomeClass {
public:
    void SomeMethod5() const {
        std::cout << turbo::nameof_type<T>() << std::endl;
    }

    template<typename C>
    C SomeMethod6() const {
        C t{};
        std::cout << REFLECT_TYPE_EXPR(t) << std::endl;
        return t;
    }
};

struct Long {
    struct LL {
        int field = 0;
    };
    LL ll;
};

enum class Color {
    RED, GREEN, BLUE
};

enum AnimalFlags {
    HasClaws = 1 << 0, CanFly = 1 << 1, EatsFish = 1 << 2, Endangered = 1 << 3
};

SomeStruct structvar;
Long othervar;
SomeStruct *ptrvar = &structvar;

void name_to_chars(const char *name) {
    std::cout << name << std::endl;
}

void name_to_string(const std::string &name) {
    std::cout << name << std::endl;
}

void name_to_string_view(std::string_view name) {
    std::cout << name << std::endl;
}

int main() {
    // Compile-time.
    constexpr auto name = REFLECT(structvar);
    using namespace std::literals::string_view_literals;
    static_assert("structvar"sv == name);

    name_to_chars(name.c_str()); // 'structvar'
    // or name_to_chars(name.data());
    // Note: c_str() return name as null-terminated C string, no memory allocation.

    name_to_string(name.str()); // 'structvar'
    // Note: str() occure memory allocation to copy name to std::string.
    // or name_to_string(std::string{name});
    // or name_to_string(static_cast<std::string>(name));
    // Note: cast to std::string occure memory allocation to copy name to std::string.

    name_to_string_view(name); // 'structvar'
    // Note: Implicit cast to std::string_view, no memory allocation.

#if defined(REFLECT_ENUM_SUPPORTED)
    // Nameof enum variable.
    auto color = Color::RED;
    std::cout << turbo::nameof_enum(color) << std::endl; // 'RED'
    std::cout << REFLECT_ENUM(color) << std::endl; // 'RED'
    std::cout << turbo::nameof_enum<Color::GREEN>() << std::endl; // 'GREEN'

    // Nameof enum flags.
    auto flag = static_cast<AnimalFlags>(AnimalFlags::CanFly | AnimalFlags::EatsFish);
    std::cout << turbo::nameof_enum_flag(flag) << std::endl; // 'CanFly|EatsFish'
    std::cout << REFLECT_ENUM_FLAG(flag) << std::endl; // 'CanFly|EatsFish'
#endif

    // Nameof.
    std::cout << REFLECT(structvar) << std::endl; // 'structvar'
    std::cout << REFLECT(::structvar) << std::endl; // 'structvar'
    std::cout << REFLECT(structvar.somefield) << std::endl; // 'somefield'
    std::cout << REFLECT((&structvar)->somefield) << std::endl; // 'somefield'
    std::cout << REFLECT(othervar.ll.field) << std::endl; // 'field'
    std::cout << REFLECT(ptrvar) << std::endl; // 'ptrvar

    // Nameof function.
    std::cout << REFLECT(&SomeStruct::SomeMethod1) << std::endl; // 'SomeMethod1'
    std::cout << REFLECT(structvar.SomeMethod2()) << std::endl; // 'SomeMethod2'
    std::cout << REFLECT(SomeMethod3) << std::endl; // 'SomeMethod3'
    std::cout << REFLECT(SomeMethod4<int, float>(1.0f)) << std::endl; // 'SomeMethod4'
    std::cout << REFLECT(&SomeClass<int>::SomeMethod5) << std::endl; // 'SomeMethod5'
    std::cout << REFLECT(&SomeClass<int>::SomeMethod6<long int>) << std::endl; // 'SomeMethod6'

    // Nameof with template suffix.
    std::cout << REFLECT_FULL(SomeMethod4<int, float>) << std::endl; // 'SomeMethod4<int, float>'
    std::cout << REFLECT_FULL(&SomeClass<int>::SomeMethod6<long int>) << std::endl; // 'SomeMethod6<long int>'

    // Nameof type.
    std::cout << turbo::nameof_type<const Long::LL &>() << std::endl; // 'Long::LL'
    std::cout << REFLECT_TYPE(const Long::LL&) << std::endl; // 'Long::LL'
    std::cout << turbo::nameof_full_type<const Long::LL &>() << std::endl; // 'const Long::LL &'
    std::cout << REFLECT_FULL_TYPE(const Long::LL&) << std::endl; // 'const Long::LL &'
    std::cout << REFLECT_SHORT_TYPE(const Long::LL&) << std::endl; // 'LL'
    std::cout << REFLECT_SHORT_TYPE(const SomeClass<int>&) << std::endl; // 'SomeClass'

    // Nameof variable type.
    std::cout << turbo::nameof_type<decltype(structvar)>() << std::endl; // 'SomeStruct'
    std::cout << REFLECT_TYPE_EXPR(structvar) << std::endl; // 'SomeStruct'
    std::cout << REFLECT_TYPE_EXPR(std::declval<const SomeClass<int>>()) << std::endl; // 'SomeClass<int>'
    std::cout << REFLECT_FULL_TYPE_EXPR(std::declval<const SomeClass<int>>()) << std::endl; // 'const SomeClass<int> &&'
    std::cout << REFLECT_SHORT_TYPE_EXPR(std::declval<const SomeClass<int>>()) << std::endl; // 'SomeClass'

#if defined(REFLECT_MEMBER_SUPPORTED)
    // Nameof member
    std::cout << turbo::nameof_member<&SomeStruct::somefield>() << std::endl; // somefield
    std::cout << turbo::nameof_member<&SomeStruct::SomeMethod1>() << std::endl; // SomeMethod1
    std::cout << REFLECT_MEMBER(&Long::LL::field) << std::endl; // field
    constexpr auto member_ptr = &SomeStruct::somefield;
    std::cout << REFLECT_MEMBER(member_ptr) << std::endl; // somefield
#endif

    // Nameof macro.
    std::cout << REFLECT(__LINE__) << std::endl; // '__LINE__'

    // Nameof raw.
    std::cout << REFLECT_RAW(structvar.somefield) << std::endl; // 'structvar.somefield'
    std::cout << REFLECT_RAW(&SomeStruct::SomeMethod1) << std::endl; // '&SomeStruct::SomeMethod1'

#if defined(REFLECT_TYPE_RTTI_SUPPORTED)
    // Nameof type using RTTI.
    Base *ptr = new Derived();
    std::cout << REFLECT_TYPE_RTTI(ptr) << std::endl; // 'Base *'
    std::cout << REFLECT_TYPE_RTTI(*ptr) << std::endl; // 'Derived'
#endif

    // Some more complex example.

    std::cout << SomeMethod4<int>(structvar) << std::endl; // 'SomeMethod4<int, SomeStruct>(SomeStruct value)'

    auto div = [](int x, int y) -> int {
        if (y == 0) {
            throw std::invalid_argument(REFLECT(y).str() + " should not be zero!");
        }
        return x / y;
    };

    try {
        auto z = div(10, 0);
        std::cout << z << std::endl;
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl; // 'y should not be zero!'
    }

    /* Remarks */
#if 0
    // This expression does not have name.
    std::cout << REFLECT(ptrvar[0]) << std::endl; // ''
    std::cout << REFLECT(42.0) << std::endl; // ''
    std::cout << REFLECT(42) << std::endl; // ''
    std::cout << REFLECT(42.0_deg) << std::endl; // ''
    std::cout << REFLECT((structvar)) << std::endl; // ''
    std::cout << REFLECT((SomeMethod4<int, float>)(1.0f)) << std::endl; // ''
    std::cout << REFLECT(42, 42, 42) << std::endl; // ''
    std::cout << REFLECT(42 + 42) << std::endl; // ''
    std::cout << REFLECT("Bad case"_string) << std::endl; // ''
    std::cout << REFLECT("Bad case") << std::endl; // ''
    std::cout << REFLECT("somevar.somefield") << std::endl; // ''
    std::cout << REFLECT(42.f) << std::endl; // ''
    std::cout << REFLECT(structvar.somefield++) << std::endl; // ''
    std::cout << REFLECT(std::string{"test"}) << std::endl; // ''
#endif

    return 0;
}
