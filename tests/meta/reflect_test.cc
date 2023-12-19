// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2023 Daniil Goncharov <neargye@gmail.com>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#define USER_REFLECT_ENUM_RANGE_MIN -120
#define USER_REFLECT_ENUM_RANGE_MAX 120

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"


#include "turbo/meta/reflect.h"

#include <string>
#include <stdexcept>

#ifdef NDEBUG
#  define REFLECT_DEBUG_REQUIRE(...) REQUIRE(__VA_ARGS__)
#else
#  define REFLECT_DEBUG_REQUIRE(...)
#endif

struct SomeStruct {
    int somefield = 0;

    void SomeMethod1(int) {
        throw std::runtime_error{"should not be called!"};
    }

    int SomeMethod2() const {
        throw std::runtime_error{"should not be called!"};
    }

    static int somestaticfield;
    constexpr static int someotherstaticfield = 21;
};

int SomeStruct::somestaticfield;

int someglobalvariable = 0;
const int someglobalconstvariable = 42;

void SomeMethod3() {
    throw std::runtime_error{"should not be called!"};
}

template<typename T, typename U>
std::string SomeMethod4(U) {
    throw std::runtime_error{"should not be called!"};
}

template<typename T>
class SomeClass {
public:
    void SomeMethod5() const {
        throw std::runtime_error{"should not be called!"};
    }

    template<typename C>
    C SomeMethod6() const {
        throw std::runtime_error{"should not be called!"};
    }
};

struct Long {
    struct LL {
        int field = 0;
    };
    LL ll;
};

enum class Color {
    RED = -12, GREEN = 7, BLUE = 15
};

enum class Numbers : int {
    one = 1, two, three, many = 127
};

enum Directions {
    Up = 85, Down = -42, Right = 120, Left = -120
};

enum number : unsigned long {
    one = 100, two = 200, three = 300, four = 400
};

enum AnimalFlags {
    HasClaws = 1,
    CanFly = 2,
    EatsFish = 4,
    Endangered = 8,
};

enum class BigFlags : std::uint64_t {
    A = 1,
    B = (static_cast<std::uint64_t>(0x1) << 20),
    C = (static_cast<std::uint64_t>(0x1) << 40),
    D = (static_cast<std::uint64_t>(0x1) << 63),
};

template<>
struct turbo::customize::enum_range<number> {
    static_assert(std::is_enum_v<number>, "turbo::enum_range<number> requires enum type.");
    static constexpr int min = 100;
    static constexpr int max = 300;
    static_assert(max > min, "turbo::enum_range<number> requires max > min.");
};

enum class OutOfRange {
    too_low = REFLECT_ENUM_RANGE_MIN - 1,
    required_to_work = 0,
    too_high = REFLECT_ENUM_RANGE_MAX + 1
};

struct TestRtti {
    struct Base {
        virtual ~Base() = default;
    };

    struct Derived : Base {
    };
};

SomeStruct struct_var;
Long othervar;
SomeStruct *ptr_s = &struct_var;
SomeStruct &ref_s = struct_var;

SomeClass<int> class_var;
const SomeClass<int> volatile *ptr_c = nullptr;

const Color color = Color::RED;

TEST_CASE("REFLECTION, variable") {
    constexpr auto name = REFLECT(othervar);
    REQUIRE(name == "othervar");
    REQUIRE(REFLECT(struct_var) == "struct_var");
    REQUIRE(REFLECT(::struct_var) == "struct_var");
    REQUIRE(REFLECT(ptr_s) == "ptr_s");
    REQUIRE(REFLECT(color) == "color");
}

TEST_CASE("REFLECTION, member") {
    REQUIRE(REFLECT(struct_var.somefield) == "somefield");
    REQUIRE(REFLECT((&struct_var)->somefield) == "somefield");
    REQUIRE(REFLECT(othervar.ll.field) == "field");
}

TEST_CASE("REFLECTION, function") {
    REQUIRE(REFLECT(&SomeStruct::SomeMethod1) == "SomeMethod1");
    REQUIRE(REFLECT(struct_var.SomeMethod1(1)) == "SomeMethod1");
    REQUIRE(REFLECT(&SomeStruct::SomeMethod2) == "SomeMethod2");
    REQUIRE(REFLECT(struct_var.SomeMethod2()) == "SomeMethod2");
    REQUIRE(REFLECT(SomeMethod3) == "SomeMethod3");
    REQUIRE(REFLECT(SomeMethod3()) == "SomeMethod3");
    REQUIRE(REFLECT(SomeMethod4<int, float>) == "SomeMethod4");
    REQUIRE(REFLECT(SomeMethod4<int, float>(1.0f)) == "SomeMethod4");
    REQUIRE(REFLECT(&SomeClass<int>::SomeMethod5) == "SomeMethod5");
    REQUIRE(REFLECT(class_var.SomeMethod5()) == "SomeMethod5");
    REQUIRE(REFLECT(&SomeClass<int>::SomeMethod6<long int>) == "SomeMethod6");
    REQUIRE(REFLECT(class_var.SomeMethod6<long int>()) == "SomeMethod6");
}

TEST_CASE("REFLECTION, enum") {
    REQUIRE(REFLECT(Color::RED) == "RED");
    REQUIRE(REFLECT(Color::BLUE) == "BLUE");
}

TEST_CASE("REFLECTION_FULL, variable") {
    constexpr auto full_name = REFLECT_FULL(othervar);
    REQUIRE(full_name == "othervar");
    REQUIRE(REFLECT_FULL(struct_var) == "struct_var");
    REQUIRE(REFLECT_FULL(::struct_var) == "struct_var");
    REQUIRE(REFLECT_FULL(ptr_s) == "ptr_s");
    REQUIRE(REFLECT_FULL(color) == "color");
}

TEST_CASE("REFLECTION_FULL, member") {
    REQUIRE(REFLECT_FULL(struct_var.somefield) == "somefield");
    REQUIRE(REFLECT_FULL((&struct_var)->somefield) == "somefield");
    REQUIRE(REFLECT_FULL(othervar.ll.field) == "field");
}

TEST_CASE("REFLECTION_FULL, function") {
    REQUIRE(REFLECT_FULL(&SomeStruct::SomeMethod1) == "SomeMethod1");
    REQUIRE(REFLECT_FULL(struct_var.SomeMethod1(1)) == "SomeMethod1");
    REQUIRE(REFLECT_FULL(&SomeStruct::SomeMethod2) == "SomeMethod2");
    REQUIRE(REFLECT_FULL(struct_var.SomeMethod2()) == "SomeMethod2");
    REQUIRE(REFLECT_FULL(SomeMethod3) == "SomeMethod3");
    REQUIRE(REFLECT_FULL(SomeMethod3()) == "SomeMethod3");
    REQUIRE(REFLECT_FULL(SomeMethod4<int, float>) == "SomeMethod4<int, float>");
    REQUIRE(REFLECT_FULL(SomeMethod4<int, float>(1.0f)) == "SomeMethod4<int, float>");
    REQUIRE(REFLECT_FULL(&SomeClass<int>::SomeMethod5) == "SomeMethod5");
    REQUIRE(REFLECT_FULL(class_var.SomeMethod5()) == "SomeMethod5");
    REQUIRE(REFLECT_FULL(&SomeClass<int>::SomeMethod6<long int>) == "SomeMethod6<long int>");
    REQUIRE(REFLECT_FULL(class_var.SomeMethod6<long int>()) == "SomeMethod6<long int>");
}

TEST_CASE("REFLECTION_FULL, enum") {
    REQUIRE(REFLECT_FULL(Color::RED) == "RED");
    REQUIRE(REFLECT_FULL(Color::BLUE) == "BLUE");
}


TEST_CASE("REFLECTION_RAW, variable") {
    constexpr auto raw_name = REFLECT_RAW(othervar);
    REQUIRE(raw_name == "othervar");
    REQUIRE(REFLECT_RAW(struct_var) == "struct_var");
    REQUIRE(REFLECT_RAW(&struct_var) == "&struct_var");
    REQUIRE(REFLECT_RAW(::struct_var) == "::struct_var");
    REQUIRE(REFLECT_RAW(ptr_s) == "ptr_s");
    REQUIRE(REFLECT_RAW(*ptr_s) == "*ptr_s");
    REQUIRE(REFLECT_RAW(ptr_s[0]) == "ptr_s[0]");
    REQUIRE(REFLECT_RAW(color) == "color");
}

TEST_CASE("REFLECTION_RAW, member") {
    REQUIRE(REFLECT_RAW(struct_var.somefield) == "struct_var.somefield");
    REQUIRE(REFLECT_RAW(struct_var.somefield++) == "struct_var.somefield++");
    REQUIRE(REFLECT_RAW((&struct_var)->somefield) == "(&struct_var)->somefield");
    REQUIRE(REFLECT_RAW(othervar.ll.field) == "othervar.ll.field");
    REQUIRE(REFLECT_RAW(+struct_var.somefield) == "+struct_var.somefield");
    REQUIRE(REFLECT_RAW(-struct_var.somefield) == "-struct_var.somefield");
    REQUIRE(REFLECT_RAW(~struct_var.somefield) == "~struct_var.somefield");
    REQUIRE(REFLECT_RAW(!struct_var.somefield) == "!struct_var.somefield");
    REQUIRE(REFLECT_RAW(struct_var.somefield + ref_s.somefield) == "struct_var.somefield + ref_s.somefield");
}

TEST_CASE("REFLECTION_RAW, function") {
    REQUIRE(REFLECT_RAW(&SomeStruct::SomeMethod1) == "&SomeStruct::SomeMethod1");
    REQUIRE(REFLECT_RAW(struct_var.SomeMethod1(1)) == "struct_var.SomeMethod1(1)");
    REQUIRE(REFLECT_RAW(&SomeStruct::SomeMethod2) == "&SomeStruct::SomeMethod2");
    REQUIRE(REFLECT_RAW(struct_var.SomeMethod2()) == "struct_var.SomeMethod2()");
    REQUIRE(REFLECT_RAW(SomeMethod3) == "SomeMethod3");
    REQUIRE(REFLECT_RAW(SomeMethod3()) == "SomeMethod3()");
    REQUIRE(REFLECT_RAW(SomeMethod4<int, float>) == "SomeMethod4<int, float>");
    REQUIRE(REFLECT_RAW(SomeMethod4<int, float>(1.0f)) == "SomeMethod4<int, float>(1.0f)");
    REQUIRE(REFLECT_RAW(&SomeClass<int>::SomeMethod5) == "&SomeClass<int>::SomeMethod5");
    REQUIRE(REFLECT_RAW(class_var.SomeMethod5()) == "class_var.SomeMethod5()");
    REQUIRE(REFLECT_RAW(&SomeClass<int>::SomeMethod6<long int>) == "&SomeClass<int>::SomeMethod6<long int>");
    REQUIRE(REFLECT_RAW(class_var.SomeMethod6<long int>()) == "class_var.SomeMethod6<long int>()");
}

TEST_CASE("REFLECTION_RAW, enum") {
    REQUIRE(REFLECT_RAW(Color::RED) == "Color::RED");
    REQUIRE(REFLECT_RAW(Color::BLUE) == "Color::BLUE");
}

TEST_CASE("REFLECTION_RAW, macro") {
    REQUIRE(REFLECT_RAW(__cplusplus) == "__cplusplus");
    REQUIRE(REFLECT_RAW(__LINE__) == "__LINE__");
    REQUIRE(REFLECT_RAW(__FILE__) == "__FILE__");
}

#if defined(REFLECT_ENUM_SUPPORTED)

static_assert(turbo::is_nameof_enum_supported,
              "turbo::nameof_enum: Unsupported compiler (https://github.com/Neargye/nameof#compiler-compatibility).");

TEST_CASE("REFLECTION_ENUM, automatic_storage") {
    constexpr Color cr = Color::RED;
    constexpr auto cr_name = turbo::nameof_enum(cr);
    Color cm[3] = {Color::RED, Color::GREEN, Color::BLUE};
    REQUIRE(cr_name == "RED");
    REQUIRE(turbo::nameof_enum(Color::BLUE) == "BLUE");
    REQUIRE(turbo::nameof_enum(cm[1]) == "GREEN");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(static_cast<Color>(0)).empty());

    constexpr Numbers no = Numbers::one;
    constexpr auto no_name = turbo::nameof_enum(no);
    REQUIRE(no_name == "one");
    REQUIRE(turbo::nameof_enum(Numbers::two) == "two");
    REQUIRE(turbo::nameof_enum(Numbers::three) == "three");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(Numbers::many).empty());
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(static_cast<Numbers>(0)).empty());

    constexpr Directions dr = Directions::Right;
    constexpr auto dr_name = turbo::nameof_enum(dr);
    REQUIRE(turbo::nameof_enum(Directions::Up) == "Up");
    REQUIRE(turbo::nameof_enum(Directions::Down) == "Down");
    REQUIRE(dr_name == "Right");
    REQUIRE(turbo::nameof_enum(Directions::Left) == "Left");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(static_cast<Directions>(0)).empty());

    constexpr number nt = number::three;
    constexpr auto nt_name = turbo::nameof_enum(nt);
    REQUIRE(turbo::nameof_enum(number::one) == "one");
    REQUIRE(turbo::nameof_enum(number::two) == "two");
    REQUIRE(nt_name == "three");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(number::four).empty());
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(static_cast<number>(0)).empty());
}

TEST_CASE("REFLECTION_ENUM, static_storage") {
    constexpr Color cr = Color::RED;
    constexpr auto cr_name = turbo::nameof_enum<cr>();
    constexpr Color cm[3] = {Color::RED, Color::GREEN, Color::BLUE};
    REQUIRE(cr_name == "RED");
    REQUIRE(turbo::nameof_enum<Color::BLUE>() == "BLUE");
    REQUIRE(turbo::nameof_enum<cm[1]>() == "GREEN");

    constexpr Numbers no = Numbers::one;
    constexpr auto no_name = turbo::nameof_enum<no>();
    REQUIRE(no_name == "one");
    REQUIRE(turbo::nameof_enum<Numbers::two>() == "two");
    REQUIRE(turbo::nameof_enum<Numbers::three>() == "three");
    REQUIRE(turbo::nameof_enum<Numbers::many>() == "many");

    constexpr Directions dr = Directions::Right;
    constexpr auto dr_name = turbo::nameof_enum<dr>();
    REQUIRE(turbo::nameof_enum<Directions::Up>() == "Up");
    REQUIRE(turbo::nameof_enum<Directions::Down>() == "Down");
    REQUIRE(dr_name == "Right");
    REQUIRE(turbo::nameof_enum<Directions::Left>() == "Left");

    constexpr number nt = number::three;
    constexpr auto nt_name = turbo::nameof_enum<nt>();
    REQUIRE(turbo::nameof_enum<number::one>() == "one");
    REQUIRE(turbo::nameof_enum<number::two>() == "two");
    REQUIRE(nt_name == "three");
    REQUIRE(turbo::nameof_enum<number::four>() == "four");
}

TEST_CASE("REFLECTION_ENUM, nameof_enum_flag") {
    constexpr AnimalFlags af = AnimalFlags::HasClaws;
    auto af_name = turbo::nameof_enum_flag(af);
    AnimalFlags afm[3] = {AnimalFlags::HasClaws, AnimalFlags::CanFly, AnimalFlags::EatsFish};
    REQUIRE(af_name == "HasClaws");
    REQUIRE(turbo::nameof_enum_flag(AnimalFlags::EatsFish) == "EatsFish");
    REQUIRE(turbo::nameof_enum_flag(afm[1]) == "CanFly");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(0)).empty());
    REQUIRE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(1 | 2)) == "HasClaws|CanFly");
    REQUIRE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(1 | 2 | 4)) == "HasClaws|CanFly|EatsFish");
    REQUIRE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(1 | 0 | 8)) == "HasClaws|Endangered");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(0)).empty());

    constexpr BigFlags bf = BigFlags::A;
    auto bf_name = turbo::nameof_enum_flag(bf);
    BigFlags bfm[3] = {BigFlags::A, BigFlags::B, BigFlags::C};
    REQUIRE(bf_name == "A");
    REQUIRE(turbo::nameof_enum_flag(BigFlags::C) == "C");
    REQUIRE(turbo::nameof_enum_flag(bfm[1]) == "B");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>(0)).empty());
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>(1 | 2)).empty());
    REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>(1 | (static_cast<std::uint64_t>(0x1) << 20))) == "A|B");
    REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>(1 | (static_cast<std::uint64_t>(0x1) << 20) |
                                                          (static_cast<std::uint64_t>(0x1) << 63))) == "A|B|D");
    REQUIRE(
            turbo::nameof_enum_flag(static_cast<BigFlags>(1 | 0 | (static_cast<std::uint64_t>(0x1) << 40))) == "A|C");
    REQUIRE(
            turbo::nameof_enum_flag(static_cast<BigFlags>(1 | 0 | (static_cast<std::uint64_t>(0x1) << 40))) == "A|C");
    REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>((static_cast<std::uint64_t>(0x1) << 63) | 1)) == "A|D");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>(2)).empty());
    REFLECT_DEBUG_REQUIRE(
            turbo::nameof_enum_flag(static_cast<BigFlags>((static_cast<std::uint64_t>(0x1) << 63) | 2)).empty());
}

TEST_CASE("REFLECTION_ENUM, REFLECT_ENUM") {
    constexpr Color cr = Color::RED;
    constexpr auto cr_name = REFLECT_ENUM(cr);
    Color cm[3] = {Color::RED, Color::GREEN, Color::BLUE};
    REQUIRE(cr_name == "RED");
    REQUIRE(REFLECT_ENUM(Color::BLUE) == "BLUE");
    REQUIRE(REFLECT_ENUM(cm[1]) == "GREEN");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(static_cast<Color>(0)).empty());

    constexpr Numbers no = Numbers::one;
    constexpr auto no_name = REFLECT_ENUM(no);
    REQUIRE(no_name == "one");
    REQUIRE(REFLECT_ENUM(Numbers::two) == "two");
    REQUIRE(REFLECT_ENUM(Numbers::three) == "three");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(Numbers::many).empty());
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(static_cast<Numbers>(0)).empty());

    constexpr Directions dr = Directions::Right;
    constexpr auto dr_name = REFLECT_ENUM(dr);
    REQUIRE(REFLECT_ENUM(Directions::Up) == "Up");
    REQUIRE(REFLECT_ENUM(Directions::Down) == "Down");
    REQUIRE(dr_name == "Right");
    REQUIRE(REFLECT_ENUM(Directions::Left) == "Left");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(static_cast<Directions>(0)).empty());

    constexpr number nt = number::three;
    constexpr auto nt_name = REFLECT_ENUM(nt);
    REQUIRE(REFLECT_ENUM(number::one) == "one");
    REQUIRE(REFLECT_ENUM(number::two) == "two");
    REQUIRE(nt_name == "three");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(number::four).empty());
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(static_cast<number>(0)).empty());
}

TEST_CASE("REFLECTION_ENUM, REFLECT_ENUM_CONST") {
    constexpr Color cr = Color::RED;
    constexpr auto cr_name = REFLECT_ENUM_CONST(cr);
    constexpr Color cm[3] = {Color::RED, Color::GREEN, Color::BLUE};
    REQUIRE(cr_name == "RED");
    REQUIRE(REFLECT_ENUM_CONST(Color::BLUE) == "BLUE");
    REQUIRE(REFLECT_ENUM_CONST(cm[1]) == "GREEN");

    constexpr Numbers no = Numbers::one;
    constexpr auto no_name = REFLECT_ENUM_CONST(no);
    REQUIRE(no_name == "one");
    REQUIRE(REFLECT_ENUM_CONST(Numbers::two) == "two");
    REQUIRE(REFLECT_ENUM_CONST(Numbers::three) == "three");
    REQUIRE(REFLECT_ENUM_CONST(Numbers::many) == "many");

    constexpr Directions dr = Directions::Right;
    constexpr auto dr_name = REFLECT_ENUM_CONST(dr);
    REQUIRE(REFLECT_ENUM_CONST(Directions::Up) == "Up");
    REQUIRE(REFLECT_ENUM_CONST(Directions::Down) == "Down");
    REQUIRE(dr_name == "Right");
    REQUIRE(REFLECT_ENUM_CONST(Directions::Left) == "Left");

    constexpr number nt = number::three;
    constexpr auto nt_name = REFLECT_ENUM_CONST(nt);
    REQUIRE(REFLECT_ENUM_CONST(number::one) == "one");
    REQUIRE(REFLECT_ENUM_CONST(number::two) == "two");
    REQUIRE(nt_name == "three");
    REQUIRE(REFLECT_ENUM_CONST(number::four) == "four");
}

TEST_CASE("REFLECTION_ENUM, REFLECT_ENUM_FLAG") {
    constexpr AnimalFlags af = AnimalFlags::HasClaws;
    auto af_name = REFLECT_ENUM_FLAG(af);
    AnimalFlags afm[3] = {AnimalFlags::HasClaws, AnimalFlags::CanFly, AnimalFlags::EatsFish};
    REQUIRE(af_name == "HasClaws");
    REQUIRE(REFLECT_ENUM_FLAG(afm[1]) == "CanFly");
    REQUIRE(REFLECT_ENUM_FLAG(AnimalFlags::EatsFish) == "EatsFish");
    REQUIRE(REFLECT_ENUM_FLAG(AnimalFlags::Endangered) == "Endangered");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(0)).empty());
    REQUIRE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(1 | 2)) == "HasClaws|CanFly");
    REQUIRE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(1 | 2 | 4)) == "HasClaws|CanFly|EatsFish");
    REQUIRE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(1 | 0 | 8)) == "HasClaws|Endangered");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(0)).empty());

    constexpr BigFlags bf = BigFlags::A;
    auto bf_name = REFLECT_ENUM_FLAG(bf);
    BigFlags bfm[3] = {BigFlags::A, BigFlags::B, BigFlags::C};
    REQUIRE(bf_name == "A");
    REQUIRE(REFLECT_ENUM_FLAG(bfm[1]) == "B");
    REQUIRE(REFLECT_ENUM_FLAG(BigFlags::C) == "C");
    REQUIRE(REFLECT_ENUM_FLAG(BigFlags::D) == "D");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(0)).empty());
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(1 | 2)).empty());
    REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(1 | (static_cast<std::uint64_t>(0x1) << 20))) == "A|B");
    REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(1 | (static_cast<std::uint64_t>(0x1) << 20) |
                                                    (static_cast<std::uint64_t>(0x1) << 63))) == "A|B|D");
    REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>((static_cast<std::uint64_t>(0x1) << 63) | 1)) == "A|D");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(2)).empty());
    REFLECT_DEBUG_REQUIRE(
            REFLECT_ENUM_FLAG(static_cast<BigFlags>((static_cast<std::uint64_t>(0x1) << 63) | 2)).empty());
}

TEST_CASE("REFLECTION_ENUM, nameof_enum_or") {
    OutOfRange low = OutOfRange::too_low;
    OutOfRange high = OutOfRange::too_high;
    auto low_name = turbo::nameof_enum_or(low, "-121");
    auto high_name = turbo::nameof_enum_or(high, "121");
    constexpr OutOfRange oor[] = {OutOfRange::too_high, OutOfRange::too_low};
    REQUIRE(low_name == "-121");
    REQUIRE(high_name == "121");
    REQUIRE(turbo::nameof_enum_or(oor[0], "121") == "121");
}

TEST_CASE("REFLECTION_ENUM, REFLECT_ENUM_OR") {
    OutOfRange low = OutOfRange::too_low;
    OutOfRange high = OutOfRange::too_high;
    auto low_name = REFLECT_ENUM_OR(low, "-121");
    auto high_name = REFLECT_ENUM_OR(high, "121");
    constexpr OutOfRange oor[] = {OutOfRange::too_high, OutOfRange::too_low};
    REQUIRE(low_name == "-121");
    REQUIRE(high_name == "121");
    REQUIRE(REFLECT_ENUM_OR(oor[0], "121") == "121");
}

#endif

#if defined(REFLECT_TYPE_SUPPORTED)

static_assert(turbo::is_nameof_type_supported,
              "turbo::nameof_type: Unsupported compiler (https://github.com/Neargye/nameof#compiler-compatibility).");

TEST_CASE("REFLECTION_ENUM, nameof_nameof_type") {
    constexpr auto type_name = turbo::nameof_type<decltype(struct_var)>();
#if defined(__clang__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(turbo::nameof_type<decltype(ptr_s)>() == "SomeStruct *");
    REQUIRE(turbo::nameof_type<decltype(ref_s)>() == "SomeStruct");
    REQUIRE(turbo::nameof_type<SomeStruct>() == "SomeStruct");
    REQUIRE(turbo::nameof_type<SomeStruct *>() == "SomeStruct *");
    REQUIRE(turbo::nameof_type<const SomeStruct &>() == "SomeStruct");
    REQUIRE(turbo::nameof_type<const SomeStruct volatile *>() == "const volatile SomeStruct *");

    REQUIRE(turbo::nameof_type<SomeClass<int>>() == "SomeClass<int>");
    REQUIRE(turbo::nameof_type<const SomeClass<int> volatile *>() == "const volatile SomeClass<int> *");

    REQUIRE(turbo::nameof_type<decltype(othervar)>() == "Long");
    REQUIRE(turbo::nameof_type<Long>() == "Long");
    REQUIRE(turbo::nameof_type<Long::LL>() == "Long::LL");

    REQUIRE(turbo::nameof_type<Color>() == "Color");
#elif defined(_MSC_VER)
    REQUIRE(type_name == "struct SomeStruct");
    REQUIRE(turbo::nameof_type<decltype(ptr_s)>() == "struct SomeStruct *");
    REQUIRE(turbo::nameof_type<decltype(ref_s)>() == "struct SomeStruct");
    REQUIRE(turbo::nameof_type<SomeStruct>() == "struct SomeStruct");
    REQUIRE(turbo::nameof_type<SomeStruct *>() == "struct SomeStruct *");
    REQUIRE(turbo::nameof_type<const SomeStruct &>() == "struct SomeStruct");
    REQUIRE(turbo::nameof_type<const SomeStruct volatile *>() == "struct SomeStruct const volatile *");

    REQUIRE(turbo::nameof_type<SomeClass<int>>() == "class SomeClass<int>");
    REQUIRE(turbo::nameof_type<const SomeClass<int> volatile *>() == "class SomeClass<int> const volatile *");

    REQUIRE(turbo::nameof_type<decltype(othervar)>() == "struct Long");
    REQUIRE(turbo::nameof_type<Long>() == "struct Long");
    REQUIRE(turbo::nameof_type<Long::LL>() == "struct Long::LL");

    REQUIRE(turbo::nameof_type<Color>() == "enum Color");
#elif defined(__GNUC__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(turbo::nameof_type<decltype(ptr_s)>() == "SomeStruct*");
    REQUIRE(turbo::nameof_type<decltype(ref_s)>() == "SomeStruct");
    REQUIRE(turbo::nameof_type<SomeStruct>() == "SomeStruct");
    REQUIRE(turbo::nameof_type<SomeStruct *>() == "SomeStruct*");
    REQUIRE(turbo::nameof_type<const SomeStruct &>() == "SomeStruct");
    REQUIRE(turbo::nameof_type<const SomeStruct volatile *>() == "const volatile SomeStruct*");

    REQUIRE(turbo::nameof_type<SomeClass<int>>() == "SomeClass<int>");
    REQUIRE(turbo::nameof_type<const SomeClass<int> volatile *>() == "const volatile SomeClass<int>*");

    REQUIRE(turbo::nameof_type<decltype(othervar)>() == "Long");
    REQUIRE(turbo::nameof_type<Long>() == "Long");
    REQUIRE(turbo::nameof_type<Long::LL>() == "Long::LL");

    REQUIRE(turbo::nameof_type<Color>() == "Color");
#endif
}

TEST_CASE("REFLECTION_ENUM, nameof_nameof_full_type") {
    constexpr auto type_name = turbo::nameof_full_type<decltype(struct_var)>();
#if defined(__clang__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(turbo::nameof_full_type<decltype(ptr_s)>() == "SomeStruct *");
    REQUIRE(turbo::nameof_full_type<decltype(ref_s)>() == "SomeStruct &");
    REQUIRE(turbo::nameof_full_type<SomeStruct>() == "SomeStruct");
    REQUIRE(turbo::nameof_full_type<SomeStruct *>() == "SomeStruct *");
    REQUIRE(turbo::nameof_full_type<SomeStruct &>() == "SomeStruct &");
    REQUIRE(turbo::nameof_full_type<const SomeStruct volatile *>() == "const volatile SomeStruct *");

    REQUIRE(turbo::nameof_full_type<SomeClass<int>>() == "SomeClass<int>");
    REQUIRE(turbo::nameof_full_type<const SomeClass<int> volatile *>() == "const volatile SomeClass<int> *");

    REQUIRE(turbo::nameof_full_type<decltype(othervar)>() == "Long");
    REQUIRE(turbo::nameof_full_type<Long>() == "Long");
    REQUIRE(turbo::nameof_full_type<Long::LL>() == "Long::LL");

    REQUIRE(turbo::nameof_full_type<Color>() == "Color");
#elif defined(_MSC_VER)
    REQUIRE(type_name == "struct SomeStruct");
    REQUIRE(turbo::nameof_full_type<decltype(ptr_s)>() == "struct SomeStruct *");
    REQUIRE(turbo::nameof_full_type<decltype(ref_s)>() == "struct SomeStruct &");
    REQUIRE(turbo::nameof_full_type<SomeStruct>() == "struct SomeStruct");
    REQUIRE(turbo::nameof_full_type<SomeStruct *>() == "struct SomeStruct *");
    REQUIRE(turbo::nameof_full_type<SomeStruct &>() == "struct SomeStruct &");
    REQUIRE(turbo::nameof_full_type<const SomeStruct volatile *>() == "struct SomeStruct const volatile *");

    REQUIRE(turbo::nameof_full_type<SomeClass<int>>() == "class SomeClass<int>");
    REQUIRE(turbo::nameof_full_type<const SomeClass<int> volatile *>() == "class SomeClass<int> const volatile *");

    REQUIRE(turbo::nameof_full_type<decltype(othervar)>() == "struct Long");
    REQUIRE(turbo::nameof_full_type<Long>() == "struct Long");
    REQUIRE(turbo::nameof_full_type<Long::LL>() == "struct Long::LL");

    REQUIRE(turbo::nameof_full_type<Color>() == "enum Color");
#elif defined(__GNUC__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(turbo::nameof_full_type<decltype(ptr_s)>() == "SomeStruct*");
    REQUIRE(turbo::nameof_full_type<decltype(ref_s)>() == "SomeStruct&");
    REQUIRE(turbo::nameof_full_type<SomeStruct>() == "SomeStruct");
    REQUIRE(turbo::nameof_full_type<SomeStruct *>() == "SomeStruct*");
    REQUIRE(turbo::nameof_full_type<SomeStruct &>() == "SomeStruct&");
    REQUIRE(turbo::nameof_full_type<const SomeStruct volatile *>() == "const volatile SomeStruct*");

    REQUIRE(turbo::nameof_full_type<SomeClass<int>>() == "SomeClass<int>");
    REQUIRE(turbo::nameof_full_type<const SomeClass<int> volatile *>() == "const volatile SomeClass<int>*");

    REQUIRE(turbo::nameof_full_type<decltype(othervar)>() == "Long");
    REQUIRE(turbo::nameof_full_type<Long>() == "Long");
    REQUIRE(turbo::nameof_full_type<Long::LL>() == "Long::LL");

    REQUIRE(turbo::nameof_full_type<Color>() == "Color");
#endif
}

TEST_CASE("REFLECTION_ENUM, nameof_nameof_short_type") {
    constexpr auto type_name = turbo::nameof_short_type<decltype(struct_var)>();
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(turbo::nameof_short_type<decltype(ref_s)>() == "SomeStruct");
    REQUIRE(turbo::nameof_short_type<SomeStruct>() == "SomeStruct");
    REQUIRE(turbo::nameof_short_type<SomeStruct &>() == "SomeStruct");
    REQUIRE(turbo::nameof_short_type<const SomeStruct volatile>() == "SomeStruct");

    REQUIRE(turbo::nameof_short_type<SomeClass<int>>() == "SomeClass");
    REQUIRE(turbo::nameof_short_type<const SomeClass<int> volatile>() == "SomeClass");

    REQUIRE(turbo::nameof_short_type<decltype(othervar)>() == "Long");
    REQUIRE(turbo::nameof_short_type<Long>() == "Long");
    REQUIRE(turbo::nameof_short_type<Long::LL>() == "LL");

    REQUIRE(turbo::nameof_short_type<Color>() == "Color");
}

TEST_CASE("REFLECTION_ENUM, REFLECT_TYPE") {
    constexpr auto type_name = REFLECT_TYPE(decltype(struct_var));
#if defined(__clang__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_TYPE(decltype(ptr_s)) == "SomeStruct *");
    REQUIRE(REFLECT_TYPE(decltype(ref_s)) == "SomeStruct");
    REQUIRE(REFLECT_TYPE(SomeStruct) == "SomeStruct");
    REQUIRE(REFLECT_TYPE(SomeStruct *) == "SomeStruct *");
    REQUIRE(REFLECT_TYPE(const SomeStruct &) == "SomeStruct");
    REQUIRE(REFLECT_TYPE(const SomeStruct volatile *) == "const volatile SomeStruct *");

    REQUIRE(REFLECT_TYPE(SomeClass<int>) == "SomeClass<int>");
    REQUIRE(REFLECT_TYPE(const SomeClass<int> volatile *) == "const volatile SomeClass<int> *");

    REQUIRE(REFLECT_TYPE(decltype(othervar)) == "Long");
    REQUIRE(REFLECT_TYPE(Long) == "Long");
    REQUIRE(REFLECT_TYPE(Long::LL) == "Long::LL");

    REQUIRE(REFLECT_TYPE(Color) == "Color");
#elif defined(_MSC_VER)
    REQUIRE(type_name == "struct SomeStruct");
    REQUIRE(REFLECT_TYPE(decltype(ptr_s)) == "struct SomeStruct *");
    REQUIRE(REFLECT_TYPE(decltype(ref_s)) == "struct SomeStruct");
    REQUIRE(REFLECT_TYPE(SomeStruct) == "struct SomeStruct");
    REQUIRE(REFLECT_TYPE(SomeStruct *) == "struct SomeStruct *");
    REQUIRE(REFLECT_TYPE(const SomeStruct &) == "struct SomeStruct");
    REQUIRE(REFLECT_TYPE(const SomeStruct volatile *) == "struct SomeStruct const volatile *");

    REQUIRE(REFLECT_TYPE(SomeClass<int>) == "class SomeClass<int>");
    REQUIRE(REFLECT_TYPE(const SomeClass<int> volatile *) == "class SomeClass<int> const volatile *");

    REQUIRE(REFLECT_TYPE(decltype(othervar)) == "struct Long");
    REQUIRE(REFLECT_TYPE(Long) == "struct Long");
    REQUIRE(REFLECT_TYPE(Long::LL) == "struct Long::LL");

    REQUIRE(REFLECT_TYPE(Color) == "enum Color");
#elif defined(__GNUC__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_TYPE(decltype(ptr_s)) == "SomeStruct*");
    REQUIRE(REFLECT_TYPE(decltype(ref_s)) == "SomeStruct");
    REQUIRE(REFLECT_TYPE(SomeStruct) == "SomeStruct");
    REQUIRE(REFLECT_TYPE(SomeStruct * ) == "SomeStruct*");
    REQUIRE(REFLECT_TYPE(const SomeStruct &) == "SomeStruct");
    REQUIRE(REFLECT_TYPE(const SomeStruct volatile *) == "const volatile SomeStruct*");

    REQUIRE(REFLECT_TYPE(SomeClass<int>) == "SomeClass<int>");
    REQUIRE(REFLECT_TYPE(const SomeClass<int> volatile *) == "const volatile SomeClass<int>*");

    REQUIRE(REFLECT_TYPE(decltype(othervar)) == "Long");
    REQUIRE(REFLECT_TYPE(Long) == "Long");
    REQUIRE(REFLECT_TYPE(Long::LL) == "Long::LL");

    REQUIRE(REFLECT_TYPE(Color) == "Color");
#endif
}

TEST_CASE("REFLECTION_ENUM, REFLECT_TYPE_EXPR") {
    constexpr auto type_name = REFLECT_TYPE_EXPR(struct_var);
#if defined(__clang__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_TYPE_EXPR(ptr_s) == "SomeStruct *");
    REQUIRE(REFLECT_TYPE_EXPR(ref_s) == "SomeStruct");

    REQUIRE(REFLECT_TYPE_EXPR(ptr_c) == "const volatile SomeClass<int> *");

    REQUIRE(REFLECT_TYPE_EXPR(othervar) == "Long");
    REQUIRE(REFLECT_TYPE_EXPR(othervar.ll) == "Long::LL");
    REQUIRE(REFLECT_TYPE_EXPR(othervar.ll.field) == "int");

    REQUIRE(REFLECT_TYPE_EXPR(Color::RED) == "Color");

    REQUIRE(REFLECT_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "SomeClass<int>");
#elif defined(_MSC_VER)
    REQUIRE(type_name == "struct SomeStruct");
    REQUIRE(REFLECT_TYPE_EXPR(ptr_s) == "struct SomeStruct *");
    REQUIRE(REFLECT_TYPE_EXPR(ref_s) == "struct SomeStruct");

    REQUIRE(REFLECT_TYPE_EXPR(ptr_c) == "class SomeClass<int> const volatile *");

    REQUIRE(REFLECT_TYPE_EXPR(othervar) == "struct Long");
    REQUIRE(REFLECT_TYPE_EXPR(othervar.ll) == "struct Long::LL");
    REQUIRE(REFLECT_TYPE_EXPR(othervar.ll.field) == "int");

    REQUIRE(REFLECT_TYPE_EXPR(Color::RED) == "enum Color");

    REQUIRE(REFLECT_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "class SomeClass<int>");
#elif defined(__GNUC__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_TYPE_EXPR(ptr_s) == "SomeStruct*");
    REQUIRE(REFLECT_TYPE_EXPR(ref_s) == "SomeStruct");

    REQUIRE(REFLECT_TYPE_EXPR(ptr_c) == "const volatile SomeClass<int>*");

    REQUIRE(REFLECT_TYPE_EXPR(othervar) == "Long");
    REQUIRE(REFLECT_TYPE_EXPR(othervar.ll) == "Long::LL");
    REQUIRE(REFLECT_TYPE_EXPR(othervar.ll.field) == "int");

    REQUIRE(REFLECT_TYPE_EXPR(Color::RED) == "Color");

    REQUIRE(REFLECT_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "SomeClass<int>");
#endif
}

TEST_CASE("REFLECTION_ENUM, REFLECT_FULL_TYPE") {
    constexpr auto type_name = REFLECT_FULL_TYPE(decltype(struct_var));
#if defined(__clang__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE(decltype(ptr_s)) == "SomeStruct *");
    REQUIRE(REFLECT_FULL_TYPE(decltype(ref_s)) == "SomeStruct &");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct) == "SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct *) == "SomeStruct *");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct &) == "SomeStruct &");
    REQUIRE(REFLECT_FULL_TYPE(const SomeStruct volatile *) == "const volatile SomeStruct *");

    REQUIRE(REFLECT_FULL_TYPE(SomeClass<int>) == "SomeClass<int>");
    REQUIRE(REFLECT_FULL_TYPE(const SomeClass<int> volatile *) == "const volatile SomeClass<int> *");

    REQUIRE(REFLECT_FULL_TYPE(decltype(othervar)) == "Long");
    REQUIRE(REFLECT_FULL_TYPE(Long) == "Long");
    REQUIRE(REFLECT_FULL_TYPE(Long::LL) == "Long::LL");

    REQUIRE(REFLECT_FULL_TYPE(Color) == "Color");
#elif defined(_MSC_VER)
    REQUIRE(type_name == "struct SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE(decltype(ptr_s)) == "struct SomeStruct *");
    REQUIRE(REFLECT_FULL_TYPE(decltype(ref_s)) == "struct SomeStruct &");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct) == "struct SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct *) == "struct SomeStruct *");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct &) == "struct SomeStruct &");
    REQUIRE(REFLECT_FULL_TYPE(const SomeStruct volatile *) == "struct SomeStruct const volatile *");

    REQUIRE(REFLECT_FULL_TYPE(SomeClass<int>) == "class SomeClass<int>");
    REQUIRE(REFLECT_FULL_TYPE(const SomeClass<int> volatile *) == "class SomeClass<int> const volatile *");

    REQUIRE(REFLECT_FULL_TYPE(decltype(othervar)) == "struct Long");
    REQUIRE(REFLECT_FULL_TYPE(Long) == "struct Long");
    REQUIRE(REFLECT_FULL_TYPE(Long::LL) == "struct Long::LL");

    REQUIRE(REFLECT_FULL_TYPE(Color) == "enum Color");
#elif defined(__GNUC__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE(decltype(ptr_s)) == "SomeStruct*");
    REQUIRE(REFLECT_FULL_TYPE(decltype(ref_s)) == "SomeStruct&");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct) == "SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct * ) == "SomeStruct*");
    REQUIRE(REFLECT_FULL_TYPE(SomeStruct & ) == "SomeStruct&");
    REQUIRE(REFLECT_FULL_TYPE(const SomeStruct volatile *) == "const volatile SomeStruct*");

    REQUIRE(REFLECT_FULL_TYPE(SomeClass<int>) == "SomeClass<int>");
    REQUIRE(REFLECT_FULL_TYPE(const SomeClass<int> volatile *) == "const volatile SomeClass<int>*");

    REQUIRE(REFLECT_FULL_TYPE(decltype(othervar)) == "Long");
    REQUIRE(REFLECT_FULL_TYPE(Long) == "Long");
    REQUIRE(REFLECT_FULL_TYPE(Long::LL) == "Long::LL");

    REQUIRE(REFLECT_FULL_TYPE(Color) == "Color");
#endif
}

TEST_CASE("REFLECTION_ENUM, REFLECT_FULL_TYPE_EXPR") {
    constexpr auto type_name = REFLECT_FULL_TYPE_EXPR(struct_var);
#if defined(__clang__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(ptr_s) == "SomeStruct *");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(ref_s) == "SomeStruct &");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(ptr_c) == "const volatile SomeClass<int> *");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar) == "Long");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar.ll) == "Long::LL");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar.ll.field) == "int");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(Color::RED) == "Color");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "const SomeClass<int> &&");
#elif defined(_MSC_VER)
    REQUIRE(type_name == "struct SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(ptr_s) == "struct SomeStruct *");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(ref_s) == "struct SomeStruct &");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(ptr_c) == "class SomeClass<int> const volatile *");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar) == "struct Long");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar.ll) == "struct Long::LL");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar.ll.field) == "int");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(Color::RED) == "enum Color");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "class SomeClass<int> const &&");
#elif defined(__GNUC__)
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(ptr_s) == "SomeStruct*");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(ref_s) == "SomeStruct&");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(ptr_c) == "const volatile SomeClass<int>*");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar) == "Long");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar.ll) == "Long::LL");
    REQUIRE(REFLECT_FULL_TYPE_EXPR(othervar.ll.field) == "int");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(Color::RED) == "Color");

    REQUIRE(REFLECT_FULL_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "const SomeClass<int>&&");
#endif
}

TEST_CASE("REFLECTION_ENUM, REFLECT_SHORT_TYPE") {
    constexpr auto type_name = REFLECT_SHORT_TYPE(decltype(struct_var));
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_SHORT_TYPE(decltype(ref_s)) == "SomeStruct");
    REQUIRE(REFLECT_SHORT_TYPE(SomeStruct) == "SomeStruct");
    REQUIRE(REFLECT_SHORT_TYPE(SomeStruct & ) == "SomeStruct");
    REQUIRE(REFLECT_SHORT_TYPE(const SomeStruct volatile) == "SomeStruct");

    REQUIRE(REFLECT_SHORT_TYPE(SomeClass<int>) == "SomeClass");
    REQUIRE(REFLECT_SHORT_TYPE(const SomeClass<int> volatile) == "SomeClass");

    REQUIRE(REFLECT_SHORT_TYPE(decltype(othervar)) == "Long");
    REQUIRE(REFLECT_SHORT_TYPE(Long) == "Long");
    REQUIRE(REFLECT_SHORT_TYPE(Long::LL) == "LL");

    REQUIRE(REFLECT_SHORT_TYPE(Color) == "Color");
}

TEST_CASE("REFLECTION_ENUM, REFLECT_SHORT_TYPE_EXPR") {
    constexpr auto type_name = REFLECT_SHORT_TYPE_EXPR(struct_var);
    REQUIRE(type_name == "SomeStruct");
    REQUIRE(REFLECT_SHORT_TYPE_EXPR(ref_s) == "SomeStruct");

    REQUIRE(REFLECT_SHORT_TYPE_EXPR(othervar) == "Long");
    REQUIRE(REFLECT_SHORT_TYPE_EXPR(othervar.ll) == "LL");
    REQUIRE(REFLECT_SHORT_TYPE_EXPR(othervar.ll.field) == "int");

    REQUIRE(REFLECT_SHORT_TYPE_EXPR(Color::RED) == "Color");

    REQUIRE(REFLECT_SHORT_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "SomeClass");
}

#endif

#if defined(REFLECT_TYPE_RTTI_SUPPORTED) && REFLECT_TYPE_RTTI_SUPPORTED

TEST_CASE("REFLECTION_ENUM, REFLECT_TYPE_RTTI") {
    TestRtti::Base *ptr = new TestRtti::Derived();
    const TestRtti::Base &const_ref = *ptr;
    volatile TestRtti::Base &volatile_ref = *ptr;
    volatile const TestRtti::Base &cv_ref = *ptr;
#if defined(__clang__) && !defined(_MSC_VER)
    REQUIRE(REFLECT_TYPE_RTTI(*ptr) == "TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(const_ref) == "TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(volatile_ref) == "TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(cv_ref) == "TestRtti::Derived");
#elif defined(_MSC_VER)
    REQUIRE(REFLECT_TYPE_RTTI(*ptr) == "struct TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(const_ref) == "struct TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(volatile_ref) == "struct TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(cv_ref) == "struct TestRtti::Derived");
#elif defined(__GNUC__)
    REQUIRE(REFLECT_TYPE_RTTI(*ptr) == "TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(const_ref) == "TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(volatile_ref) == "TestRtti::Derived");
    REQUIRE(REFLECT_TYPE_RTTI(cv_ref) == "TestRtti::Derived");
#endif
}

TEST_CASE("REFLECTION_ENUM, REFLECT_FULL_TYPE_RTTI") {
    TestRtti::Base *ptr = new TestRtti::Derived();
    const TestRtti::Base &const_ref = *ptr;
    volatile TestRtti::Base &volatile_ref = *ptr;
    volatile const TestRtti::Base &cv_ref = *ptr;
#if defined(__clang__) && !defined(_MSC_VER)
    REQUIRE(REFLECT_FULL_TYPE_RTTI(*ptr) == "TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(const_ref) == "const TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(volatile_ref) == "volatile TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(cv_ref) == "volatile const TestRtti::Derived&");
#elif defined(_MSC_VER)
    REQUIRE(REFLECT_FULL_TYPE_RTTI(*ptr) == "struct TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(const_ref) == "const struct TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(volatile_ref) == "volatile struct TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(cv_ref) == "volatile const struct TestRtti::Derived&");
#elif defined(__GNUC__)
    REQUIRE(REFLECT_FULL_TYPE_RTTI(*ptr) == "TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(const_ref) == "const TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(volatile_ref) == "volatile TestRtti::Derived&");
    REQUIRE(REFLECT_FULL_TYPE_RTTI(cv_ref) == "volatile const TestRtti::Derived&");
#endif
}

TEST_CASE("REFLECT, NAMEOFSHORT_TYPE_RTTI") {
    TestRtti::Base *ptr = new TestRtti::Derived();
    const TestRtti::Base &const_ref = *ptr;
    volatile TestRtti::Base &volatile_ref = *ptr;
    volatile const TestRtti::Base &cv_ref = *ptr;

    REQUIRE(REFLECT_SHORT_TYPE_RTTI(*ptr) == "Derived");
    REQUIRE(REFLECT_SHORT_TYPE_RTTI(const_ref) == "Derived");
    REQUIRE(REFLECT_SHORT_TYPE_RTTI(volatile_ref) == "Derived");
    REQUIRE(REFLECT_SHORT_TYPE_RTTI(cv_ref) == "Derived");
}

#endif

#if defined(REFLECT_MEMBER_SUPPORTED) && REFLECT_MEMBER_SUPPORTED

struct StructMemberInitializationUsingNameof {
    std::string teststringfield = std::string{
            turbo::nameof_member<&StructMemberInitializationUsingNameof::teststringfield>()};
};

struct StructWithNonConstexprDestructor {
    ~StructWithNonConstexprDestructor() {}

    int somefield;
};

TEST_CASE("REFLECTION_ENUM, REFLECT_MEMBER") {
    REQUIRE(REFLECT_MEMBER(&SomeStruct::somefield) == "somefield");
    REQUIRE(REFLECT_MEMBER(&SomeStruct::SomeMethod1) == "SomeMethod1");
    REQUIRE(REFLECT_MEMBER(&Long::LL::field) == "field");
    constexpr auto member_ptr = &SomeStruct::somefield;
    REQUIRE(REFLECT_MEMBER(member_ptr) == "somefield");
    REQUIRE(REFLECT_MEMBER(&StructMemberInitializationUsingNameof::teststringfield) == "teststringfield");
    REQUIRE(REFLECT_MEMBER(&StructWithNonConstexprDestructor::somefield) == "somefield");
}

TEST_CASE("REFLECT, nameof_member") {
    REQUIRE(turbo::nameof_member<&SomeStruct::somefield>() == "somefield");
    REQUIRE(turbo::nameof_member<&SomeStruct::SomeMethod1>() == "SomeMethod1");
    REQUIRE(turbo::nameof_member<&Long::LL::field>() == "field");
    constexpr auto member_ptr = &SomeStruct::somefield;
    REQUIRE(turbo::nameof_member<member_ptr>() == "somefield");
    REQUIRE(turbo::nameof_member<&StructMemberInitializationUsingNameof::teststringfield>() == "teststringfield");
    REQUIRE(turbo::nameof_member<&StructWithNonConstexprDestructor::somefield>() == "somefield");
}

#endif

#if defined(REFLECT_POINTER_SUPPORTED) && REFLECT_POINTER_SUPPORTED

void somefunction() {}

TEST_CASE("REFLECTION_ENUM, REFLECT_POINTER") {
    REQUIRE(REFLECT_POINTER(&SomeStruct::somestaticfield) == "somestaticfield");
    REQUIRE(REFLECT_POINTER(&SomeStruct::someotherstaticfield) == "someotherstaticfield");
    REQUIRE(REFLECT_POINTER(static_cast<const char *>(nullptr)) == "nullptr");
    REQUIRE(REFLECT_POINTER(static_cast<int ***>(nullptr)) == "nullptr");
    constexpr auto global_ptr = &someglobalvariable;
    REQUIRE(REFLECT_POINTER(global_ptr) == "someglobalvariable");
    REQUIRE(REFLECT_POINTER(&someglobalconstvariable) == "someglobalconstvariable");
    REQUIRE(REFLECT_POINTER(&somefunction) == "somefunction");
}

TEST_CASE("REFLECTION_ENUM, nameof_pointer") {
    REQUIRE(turbo::nameof_pointer<&SomeStruct::somestaticfield>() == "somestaticfield");
    REQUIRE(turbo::nameof_pointer<&SomeStruct::someotherstaticfield>() == "someotherstaticfield");
    REQUIRE(turbo::nameof_pointer<static_cast<const char *>(nullptr)>() == "nullptr");
    REQUIRE(turbo::nameof_pointer<static_cast<int ***>(nullptr)>() == "nullptr");
    constexpr auto global_ptr = &someglobalvariable;
    REQUIRE(turbo::nameof_pointer<global_ptr>() == "someglobalvariable");
    REQUIRE(turbo::nameof_pointer<&someglobalconstvariable>() == "someglobalconstvariable");
    REQUIRE(turbo::nameof_pointer<&somefunction>() == "somefunction");
}

#endif
