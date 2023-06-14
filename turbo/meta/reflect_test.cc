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


#include "gtest/gtest.h"

#define REFLECT_ENUM_RANGE_MIN -120
#define REFLECT_ENUM_RANGE_MAX 120

#include "turbo/meta/reflect.h"

#include <string>
#include <stdexcept>

#ifdef NDEBUG
#  define REFLECT_DEBUG_REQUIRE(...) ASSERT_TRUE(__VA_ARGS__)
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

TEST(REFLECTION, variable) {
    constexpr auto name = REFLECT(othervar);
    ASSERT_TRUE(name == "othervar");
    ASSERT_TRUE(REFLECT(struct_var) == "struct_var");
    ASSERT_TRUE(REFLECT(::struct_var) == "struct_var");
    ASSERT_TRUE(REFLECT(ptr_s) == "ptr_s");
    ASSERT_TRUE(REFLECT(color) == "color");
}

TEST(REFLECTION, member) {
    ASSERT_TRUE(REFLECT(struct_var.somefield) == "somefield");
    ASSERT_TRUE(REFLECT((&struct_var)->somefield) == "somefield");
    ASSERT_TRUE(REFLECT(othervar.ll.field) == "field");
}

TEST(REFLECTION, function) {
    ASSERT_TRUE(REFLECT(&SomeStruct::SomeMethod1) == "SomeMethod1");
    ASSERT_TRUE(REFLECT(struct_var.SomeMethod1(1)) == "SomeMethod1");
    ASSERT_TRUE(REFLECT(&SomeStruct::SomeMethod2) == "SomeMethod2");
    ASSERT_TRUE(REFLECT(struct_var.SomeMethod2()) == "SomeMethod2");
    ASSERT_TRUE(REFLECT(SomeMethod3) == "SomeMethod3");
    ASSERT_TRUE(REFLECT(SomeMethod3()) == "SomeMethod3");
    ASSERT_TRUE(REFLECT(SomeMethod4<int, float>) == "SomeMethod4");
    ASSERT_TRUE(REFLECT(SomeMethod4<int, float>(1.0f)) == "SomeMethod4");
    ASSERT_TRUE(REFLECT(&SomeClass<int>::SomeMethod5) == "SomeMethod5");
    ASSERT_TRUE(REFLECT(class_var.SomeMethod5()) == "SomeMethod5");
    ASSERT_TRUE(REFLECT(&SomeClass<int>::SomeMethod6<long int>) == "SomeMethod6");
    ASSERT_TRUE(REFLECT(class_var.SomeMethod6<long int>()) == "SomeMethod6");
}

TEST(REFLECTION, enum) {
    ASSERT_TRUE(REFLECT(Color::RED) == "RED");
    ASSERT_TRUE(REFLECT(Color::BLUE) == "BLUE");
}

TEST(REFLECTION_FULL, variable) {
    constexpr auto full_name = REFLECT_FULL(othervar);
    ASSERT_TRUE(full_name == "othervar");
    ASSERT_TRUE(REFLECT_FULL(struct_var) == "struct_var");
    ASSERT_TRUE(REFLECT_FULL(::struct_var) == "struct_var");
    ASSERT_TRUE(REFLECT_FULL(ptr_s) == "ptr_s");
    ASSERT_TRUE(REFLECT_FULL(color) == "color");
}

TEST(REFLECTION_FULL, member) {
    ASSERT_TRUE(REFLECT_FULL(struct_var.somefield) == "somefield");
    ASSERT_TRUE(REFLECT_FULL((&struct_var)->somefield) == "somefield");
    ASSERT_TRUE(REFLECT_FULL(othervar.ll.field) == "field");
}

TEST(REFLECTION_FULL, function) {
    ASSERT_TRUE(REFLECT_FULL(&SomeStruct::SomeMethod1) == "SomeMethod1");
    ASSERT_TRUE(REFLECT_FULL(struct_var.SomeMethod1(1)) == "SomeMethod1");
    ASSERT_TRUE(REFLECT_FULL(&SomeStruct::SomeMethod2) == "SomeMethod2");
    ASSERT_TRUE(REFLECT_FULL(struct_var.SomeMethod2()) == "SomeMethod2");
    ASSERT_TRUE(REFLECT_FULL(SomeMethod3) == "SomeMethod3");
    ASSERT_TRUE(REFLECT_FULL(SomeMethod3()) == "SomeMethod3");
    ASSERT_TRUE(REFLECT_FULL(SomeMethod4<int, float>) == "SomeMethod4<int, float>");
    ASSERT_TRUE(REFLECT_FULL(SomeMethod4<int, float>(1.0f)) == "SomeMethod4<int, float>");
    ASSERT_TRUE(REFLECT_FULL(&SomeClass<int>::SomeMethod5) == "SomeMethod5");
    ASSERT_TRUE(REFLECT_FULL(class_var.SomeMethod5()) == "SomeMethod5");
    ASSERT_TRUE(REFLECT_FULL(&SomeClass<int>::SomeMethod6<long int>) == "SomeMethod6<long int>");
    ASSERT_TRUE(REFLECT_FULL(class_var.SomeMethod6<long int>()) == "SomeMethod6<long int>");
}

TEST(REFLECTION_FULL, enum) {
    ASSERT_TRUE(REFLECT_FULL(Color::RED) == "RED");
    ASSERT_TRUE(REFLECT_FULL(Color::BLUE) == "BLUE");
}


TEST(REFLECTION_RAW, variable) {
    constexpr auto raw_name = REFLECT_RAW(othervar);
    ASSERT_TRUE(raw_name == "othervar");
    ASSERT_TRUE(REFLECT_RAW(struct_var) == "struct_var");
    ASSERT_TRUE(REFLECT_RAW(&struct_var) == "&struct_var");
    ASSERT_TRUE(REFLECT_RAW(::struct_var) == "::struct_var");
    ASSERT_TRUE(REFLECT_RAW(ptr_s) == "ptr_s");
    ASSERT_TRUE(REFLECT_RAW(*ptr_s) == "*ptr_s");
    ASSERT_TRUE(REFLECT_RAW(ptr_s[0]) == "ptr_s[0]");
    ASSERT_TRUE(REFLECT_RAW(color) == "color");
}

TEST(REFLECTION_RAW, member) {
    ASSERT_TRUE(REFLECT_RAW(struct_var.somefield) == "struct_var.somefield");
    ASSERT_TRUE(REFLECT_RAW(struct_var.somefield++) == "struct_var.somefield++");
    ASSERT_TRUE(REFLECT_RAW((&struct_var)->somefield) == "(&struct_var)->somefield");
    ASSERT_TRUE(REFLECT_RAW(othervar.ll.field) == "othervar.ll.field");
    ASSERT_TRUE(REFLECT_RAW(+struct_var.somefield) == "+struct_var.somefield");
    ASSERT_TRUE(REFLECT_RAW(-struct_var.somefield) == "-struct_var.somefield");
    ASSERT_TRUE(REFLECT_RAW(~struct_var.somefield) == "~struct_var.somefield");
    ASSERT_TRUE(REFLECT_RAW(!struct_var.somefield) == "!struct_var.somefield");
    ASSERT_TRUE(REFLECT_RAW(struct_var.somefield + ref_s.somefield) == "struct_var.somefield + ref_s.somefield");
}

TEST(REFLECTION_RAW, function) {
    ASSERT_TRUE(REFLECT_RAW(&SomeStruct::SomeMethod1) == "&SomeStruct::SomeMethod1");
    ASSERT_TRUE(REFLECT_RAW(struct_var.SomeMethod1(1)) == "struct_var.SomeMethod1(1)");
    ASSERT_TRUE(REFLECT_RAW(&SomeStruct::SomeMethod2) == "&SomeStruct::SomeMethod2");
    ASSERT_TRUE(REFLECT_RAW(struct_var.SomeMethod2()) == "struct_var.SomeMethod2()");
    ASSERT_TRUE(REFLECT_RAW(SomeMethod3) == "SomeMethod3");
    ASSERT_TRUE(REFLECT_RAW(SomeMethod3()) == "SomeMethod3()");
    ASSERT_TRUE(REFLECT_RAW(SomeMethod4<int, float>) == "SomeMethod4<int, float>");
    ASSERT_TRUE(REFLECT_RAW(SomeMethod4<int, float>(1.0f)) == "SomeMethod4<int, float>(1.0f)");
    ASSERT_TRUE(REFLECT_RAW(&SomeClass<int>::SomeMethod5) == "&SomeClass<int>::SomeMethod5");
    ASSERT_TRUE(REFLECT_RAW(class_var.SomeMethod5()) == "class_var.SomeMethod5()");
    ASSERT_TRUE(REFLECT_RAW(&SomeClass<int>::SomeMethod6<long int>) == "&SomeClass<int>::SomeMethod6<long int>");
    ASSERT_TRUE(REFLECT_RAW(class_var.SomeMethod6<long int>()) == "class_var.SomeMethod6<long int>()");
}

TEST(REFLECTION_RAW, enum) {
    ASSERT_TRUE(REFLECT_RAW(Color::RED) == "Color::RED");
    ASSERT_TRUE(REFLECT_RAW(Color::BLUE) == "Color::BLUE");
}

TEST(REFLECTION_RAW, macro) {
    ASSERT_TRUE(REFLECT_RAW(__cplusplus) == "__cplusplus");
    ASSERT_TRUE(REFLECT_RAW(__LINE__) == "__LINE__");
    ASSERT_TRUE(REFLECT_RAW(__FILE__) == "__FILE__");
}

#if defined(REFLECT_ENUM_SUPPORTED)

static_assert(turbo::is_nameof_enum_supported,
              "turbo::nameof_enum: Unsupported compiler (https://github.com/Neargye/nameof#compiler-compatibility).");

TEST(REFLECTION_ENUM, automatic_storage) {
    constexpr Color cr = Color::RED;
    constexpr auto cr_name = turbo::nameof_enum(cr);
    Color cm[3] = {Color::RED, Color::GREEN, Color::BLUE};
    ASSERT_TRUE(cr_name == "RED");
    ASSERT_TRUE(turbo::nameof_enum(Color::BLUE) == "BLUE");
    ASSERT_TRUE(turbo::nameof_enum(cm[1]) == "GREEN");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(static_cast<Color>(0)).empty());

    constexpr Numbers no = Numbers::one;
    constexpr auto no_name = turbo::nameof_enum(no);
    ASSERT_TRUE(no_name == "one");
    ASSERT_TRUE(turbo::nameof_enum(Numbers::two) == "two");
    ASSERT_TRUE(turbo::nameof_enum(Numbers::three) == "three");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(Numbers::many).empty());
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(static_cast<Numbers>(0)).empty());

    constexpr Directions dr = Directions::Right;
    constexpr auto dr_name = turbo::nameof_enum(dr);
    ASSERT_TRUE(turbo::nameof_enum(Directions::Up) == "Up");
    ASSERT_TRUE(turbo::nameof_enum(Directions::Down) == "Down");
    ASSERT_TRUE(dr_name == "Right");
    ASSERT_TRUE(turbo::nameof_enum(Directions::Left) == "Left");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(static_cast<Directions>(0)).empty());

    constexpr number nt = number::three;
    constexpr auto nt_name = turbo::nameof_enum(nt);
    ASSERT_TRUE(turbo::nameof_enum(number::one) == "one");
    ASSERT_TRUE(turbo::nameof_enum(number::two) == "two");
    ASSERT_TRUE(nt_name == "three");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(number::four).empty());
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum(static_cast<number>(0)).empty());
}

TEST(REFLECTION_ENUM, static_storage) {
    constexpr Color cr = Color::RED;
    constexpr auto cr_name = turbo::nameof_enum<cr>();
    constexpr Color cm[3] = {Color::RED, Color::GREEN, Color::BLUE};
    ASSERT_TRUE(cr_name == "RED");
    ASSERT_TRUE(turbo::nameof_enum<Color::BLUE>() == "BLUE");
    ASSERT_TRUE(turbo::nameof_enum<cm[1]>() == "GREEN");

    constexpr Numbers no = Numbers::one;
    constexpr auto no_name = turbo::nameof_enum<no>();
    ASSERT_TRUE(no_name == "one");
    ASSERT_TRUE(turbo::nameof_enum<Numbers::two>() == "two");
    ASSERT_TRUE(turbo::nameof_enum<Numbers::three>() == "three");
    ASSERT_TRUE(turbo::nameof_enum<Numbers::many>() == "many");

    constexpr Directions dr = Directions::Right;
    constexpr auto dr_name = turbo::nameof_enum<dr>();
    ASSERT_TRUE(turbo::nameof_enum<Directions::Up>() == "Up");
    ASSERT_TRUE(turbo::nameof_enum<Directions::Down>() == "Down");
    ASSERT_TRUE(dr_name == "Right");
    ASSERT_TRUE(turbo::nameof_enum<Directions::Left>() == "Left");

    constexpr number nt = number::three;
    constexpr auto nt_name = turbo::nameof_enum<nt>();
    ASSERT_TRUE(turbo::nameof_enum<number::one>() == "one");
    ASSERT_TRUE(turbo::nameof_enum<number::two>() == "two");
    ASSERT_TRUE(nt_name == "three");
    ASSERT_TRUE(turbo::nameof_enum<number::four>() == "four");
}

TEST(REFLECTION_ENUM, nameof_enum_flag) {
    constexpr AnimalFlags af = AnimalFlags::HasClaws;
    auto af_name = turbo::nameof_enum_flag(af);
    AnimalFlags afm[3] = {AnimalFlags::HasClaws, AnimalFlags::CanFly, AnimalFlags::EatsFish};
    ASSERT_TRUE(af_name == "HasClaws");
    ASSERT_TRUE(turbo::nameof_enum_flag(AnimalFlags::EatsFish) == "EatsFish");
    ASSERT_TRUE(turbo::nameof_enum_flag(afm[1]) == "CanFly");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(0)).empty());
    ASSERT_TRUE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(1 | 2)) == "HasClaws|CanFly");
    ASSERT_TRUE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(1 | 2 | 4)) == "HasClaws|CanFly|EatsFish");
    ASSERT_TRUE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(1 | 0 | 8)) == "HasClaws|Endangered");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<AnimalFlags>(0)).empty());

    constexpr BigFlags bf = BigFlags::A;
    auto bf_name = turbo::nameof_enum_flag(bf);
    BigFlags bfm[3] = {BigFlags::A, BigFlags::B, BigFlags::C};
    ASSERT_TRUE(bf_name == "A");
    ASSERT_TRUE(turbo::nameof_enum_flag(BigFlags::C) == "C");
    ASSERT_TRUE(turbo::nameof_enum_flag(bfm[1]) == "B");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>(0)).empty());
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>(1 | 2)).empty());
    ASSERT_TRUE(turbo::nameof_enum_flag(static_cast<BigFlags>(1 | (static_cast<std::uint64_t>(0x1) << 20))) == "A|B");
    ASSERT_TRUE(turbo::nameof_enum_flag(static_cast<BigFlags>(1 | (static_cast<std::uint64_t>(0x1) << 20) |
                                                              (static_cast<std::uint64_t>(0x1) << 63))) == "A|B|D");
    ASSERT_TRUE(
            turbo::nameof_enum_flag(static_cast<BigFlags>(1 | 0 | (static_cast<std::uint64_t>(0x1) << 40))) == "A|C");
    ASSERT_TRUE(
            turbo::nameof_enum_flag(static_cast<BigFlags>(1 | 0 | (static_cast<std::uint64_t>(0x1) << 40))) == "A|C");
    ASSERT_TRUE(turbo::nameof_enum_flag(static_cast<BigFlags>((static_cast<std::uint64_t>(0x1) << 63) | 1)) == "A|D");
    REFLECT_DEBUG_REQUIRE(turbo::nameof_enum_flag(static_cast<BigFlags>(2)).empty());
    REFLECT_DEBUG_REQUIRE(
            turbo::nameof_enum_flag(static_cast<BigFlags>((static_cast<std::uint64_t>(0x1) << 63) | 2)).empty());
}

TEST(REFLECTION_ENUM, REFLECT_ENUM) {
    constexpr Color cr = Color::RED;
    constexpr auto cr_name = REFLECT_ENUM(cr);
    Color cm[3] = {Color::RED, Color::GREEN, Color::BLUE};
    ASSERT_TRUE(cr_name == "RED");
    ASSERT_TRUE(REFLECT_ENUM(Color::BLUE) == "BLUE");
    ASSERT_TRUE(REFLECT_ENUM(cm[1]) == "GREEN");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(static_cast<Color>(0)).empty());

    constexpr Numbers no = Numbers::one;
    constexpr auto no_name = REFLECT_ENUM(no);
    ASSERT_TRUE(no_name == "one");
    ASSERT_TRUE(REFLECT_ENUM(Numbers::two) == "two");
    ASSERT_TRUE(REFLECT_ENUM(Numbers::three) == "three");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(Numbers::many).empty());
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(static_cast<Numbers>(0)).empty());

    constexpr Directions dr = Directions::Right;
    constexpr auto dr_name = REFLECT_ENUM(dr);
    ASSERT_TRUE(REFLECT_ENUM(Directions::Up) == "Up");
    ASSERT_TRUE(REFLECT_ENUM(Directions::Down) == "Down");
    ASSERT_TRUE(dr_name == "Right");
    ASSERT_TRUE(REFLECT_ENUM(Directions::Left) == "Left");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(static_cast<Directions>(0)).empty());

    constexpr number nt = number::three;
    constexpr auto nt_name = REFLECT_ENUM(nt);
    ASSERT_TRUE(REFLECT_ENUM(number::one) == "one");
    ASSERT_TRUE(REFLECT_ENUM(number::two) == "two");
    ASSERT_TRUE(nt_name == "three");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(number::four).empty());
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM(static_cast<number>(0)).empty());
}

TEST(REFLECTION_ENUM, REFLECT_ENUM_CONST) {
    constexpr Color cr = Color::RED;
    constexpr auto cr_name = REFLECT_ENUM_CONST(cr);
    constexpr Color cm[3] = {Color::RED, Color::GREEN, Color::BLUE};
    ASSERT_TRUE(cr_name == "RED");
    ASSERT_TRUE(REFLECT_ENUM_CONST(Color::BLUE) == "BLUE");
    ASSERT_TRUE(REFLECT_ENUM_CONST(cm[1]) == "GREEN");

    constexpr Numbers no = Numbers::one;
    constexpr auto no_name = REFLECT_ENUM_CONST(no);
    ASSERT_TRUE(no_name == "one");
    ASSERT_TRUE(REFLECT_ENUM_CONST(Numbers::two) == "two");
    ASSERT_TRUE(REFLECT_ENUM_CONST(Numbers::three) == "three");
    ASSERT_TRUE(REFLECT_ENUM_CONST(Numbers::many) == "many");

    constexpr Directions dr = Directions::Right;
    constexpr auto dr_name = REFLECT_ENUM_CONST(dr);
    ASSERT_TRUE(REFLECT_ENUM_CONST(Directions::Up) == "Up");
    ASSERT_TRUE(REFLECT_ENUM_CONST(Directions::Down) == "Down");
    ASSERT_TRUE(dr_name == "Right");
    ASSERT_TRUE(REFLECT_ENUM_CONST(Directions::Left) == "Left");

    constexpr number nt = number::three;
    constexpr auto nt_name = REFLECT_ENUM_CONST(nt);
    ASSERT_TRUE(REFLECT_ENUM_CONST(number::one) == "one");
    ASSERT_TRUE(REFLECT_ENUM_CONST(number::two) == "two");
    ASSERT_TRUE(nt_name == "three");
    ASSERT_TRUE(REFLECT_ENUM_CONST(number::four) == "four");
}

TEST(REFLECTION_ENUM, REFLECT_ENUM_FLAG) {
    constexpr AnimalFlags af = AnimalFlags::HasClaws;
    auto af_name = REFLECT_ENUM_FLAG(af);
    AnimalFlags afm[3] = {AnimalFlags::HasClaws, AnimalFlags::CanFly, AnimalFlags::EatsFish};
    ASSERT_TRUE(af_name == "HasClaws");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(afm[1]) == "CanFly");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(AnimalFlags::EatsFish) == "EatsFish");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(AnimalFlags::Endangered) == "Endangered");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(0)).empty());
    ASSERT_TRUE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(1 | 2)) == "HasClaws|CanFly");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(1 | 2 | 4)) == "HasClaws|CanFly|EatsFish");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(1 | 0 | 8)) == "HasClaws|Endangered");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<AnimalFlags>(0)).empty());

    constexpr BigFlags bf = BigFlags::A;
    auto bf_name = REFLECT_ENUM_FLAG(bf);
    BigFlags bfm[3] = {BigFlags::A, BigFlags::B, BigFlags::C};
    ASSERT_TRUE(bf_name == "A");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(bfm[1]) == "B");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(BigFlags::C) == "C");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(BigFlags::D) == "D");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(0)).empty());
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(1 | 2)).empty());
    ASSERT_TRUE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(1 | (static_cast<std::uint64_t>(0x1) << 20))) == "A|B");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(1 | (static_cast<std::uint64_t>(0x1) << 20) |
                                                        (static_cast<std::uint64_t>(0x1) << 63))) == "A|B|D");
    ASSERT_TRUE(REFLECT_ENUM_FLAG(static_cast<BigFlags>((static_cast<std::uint64_t>(0x1) << 63) | 1)) == "A|D");
    REFLECT_DEBUG_REQUIRE(REFLECT_ENUM_FLAG(static_cast<BigFlags>(2)).empty());
    REFLECT_DEBUG_REQUIRE(
            REFLECT_ENUM_FLAG(static_cast<BigFlags>((static_cast<std::uint64_t>(0x1) << 63) | 2)).empty());
}

TEST(REFLECTION_ENUM, nameof_enum_or) {
    OutOfRange low = OutOfRange::too_low;
    OutOfRange high = OutOfRange::too_high;
    auto low_name = turbo::nameof_enum_or(low, "-121");
    auto high_name = turbo::nameof_enum_or(high, "121");
    constexpr OutOfRange oor[] = {OutOfRange::too_high, OutOfRange::too_low};
    ASSERT_TRUE(low_name == "-121");
    ASSERT_TRUE(high_name == "121");
    ASSERT_TRUE(turbo::nameof_enum_or(oor[0], "121") == "121");
}

TEST(REFLECTION_ENUM, REFLECT_ENUM_OR) {
    OutOfRange low = OutOfRange::too_low;
    OutOfRange high = OutOfRange::too_high;
    auto low_name = REFLECT_ENUM_OR(low, "-121");
    auto high_name = REFLECT_ENUM_OR(high, "121");
    constexpr OutOfRange oor[] = {OutOfRange::too_high, OutOfRange::too_low};
    ASSERT_TRUE(low_name == "-121");
    ASSERT_TRUE(high_name == "121");
    ASSERT_TRUE(REFLECT_ENUM_OR(oor[0], "121") == "121");
}

#endif

#if defined(REFLECT_TYPE_SUPPORTED)

static_assert(turbo::is_nameof_type_supported,
              "turbo::nameof_type: Unsupported compiler (https://github.com/Neargye/nameof#compiler-compatibility).");

TEST(REFLECTION_ENUM, nameof_nameof_type) {
    constexpr auto type_name = turbo::nameof_type<decltype(struct_var)>();
#if defined(__clang__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<decltype(ptr_s)>() == "SomeStruct *");
    ASSERT_TRUE(turbo::nameof_type<decltype(ref_s)>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<SomeStruct>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<SomeStruct *>() == "SomeStruct *");
    ASSERT_TRUE(turbo::nameof_type<const SomeStruct &>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<const SomeStruct volatile *>() == "const volatile SomeStruct *");

    ASSERT_TRUE(turbo::nameof_type<SomeClass<int>>() == "SomeClass<int>");
    ASSERT_TRUE(turbo::nameof_type<const SomeClass<int> volatile *>() == "const volatile SomeClass<int> *");

    ASSERT_TRUE(turbo::nameof_type<decltype(othervar)>() == "Long");
    ASSERT_TRUE(turbo::nameof_type<Long>() == "Long");
    ASSERT_TRUE(turbo::nameof_type<Long::LL>() == "Long::LL");

    ASSERT_TRUE(turbo::nameof_type<Color>() == "Color");
#elif defined(_MSC_VER)
    ASSERT_TRUE(type_name == "struct SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<decltype(ptr_s)>() == "struct SomeStruct *");
    ASSERT_TRUE(turbo::nameof_type<decltype(ref_s)>() == "struct SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<SomeStruct>() == "struct SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<SomeStruct *>() == "struct SomeStruct *");
    ASSERT_TRUE(turbo::nameof_type<const SomeStruct &>() == "struct SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<const SomeStruct volatile *>() == "struct SomeStruct const volatile *");

    ASSERT_TRUE(turbo::nameof_type<SomeClass<int>>() == "class SomeClass<int>");
    ASSERT_TRUE(turbo::nameof_type<const SomeClass<int> volatile *>() == "class SomeClass<int> const volatile *");

    ASSERT_TRUE(turbo::nameof_type<decltype(othervar)>() == "struct Long");
    ASSERT_TRUE(turbo::nameof_type<Long>() == "struct Long");
    ASSERT_TRUE(turbo::nameof_type<Long::LL>() == "struct Long::LL");

    ASSERT_TRUE(turbo::nameof_type<Color>() == "enum Color");
#elif defined(__GNUC__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<decltype(ptr_s)>() == "SomeStruct*");
    ASSERT_TRUE(turbo::nameof_type<decltype(ref_s)>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<SomeStruct>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<SomeStruct *>() == "SomeStruct*");
    ASSERT_TRUE(turbo::nameof_type<const SomeStruct &>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_type<const SomeStruct volatile *>() == "const volatile SomeStruct*");

    ASSERT_TRUE(turbo::nameof_type<SomeClass<int>>() == "SomeClass<int>");
    ASSERT_TRUE(turbo::nameof_type<const SomeClass<int> volatile *>() == "const volatile SomeClass<int>*");

    ASSERT_TRUE(turbo::nameof_type<decltype(othervar)>() == "Long");
    ASSERT_TRUE(turbo::nameof_type<Long>() == "Long");
    ASSERT_TRUE(turbo::nameof_type<Long::LL>() == "Long::LL");

    ASSERT_TRUE(turbo::nameof_type<Color>() == "Color");
#endif
}

TEST(REFLECTION_ENUM, nameof_nameof_full_type) {
    constexpr auto type_name = turbo::nameof_full_type<decltype(struct_var)>();
#if defined(__clang__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_full_type<decltype(ptr_s)>() == "SomeStruct *");
    ASSERT_TRUE(turbo::nameof_full_type<decltype(ref_s)>() == "SomeStruct &");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct *>() == "SomeStruct *");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct &>() == "SomeStruct &");
    ASSERT_TRUE(turbo::nameof_full_type<const SomeStruct volatile *>() == "const volatile SomeStruct *");

    ASSERT_TRUE(turbo::nameof_full_type<SomeClass<int>>() == "SomeClass<int>");
    ASSERT_TRUE(turbo::nameof_full_type<const SomeClass<int> volatile *>() == "const volatile SomeClass<int> *");

    ASSERT_TRUE(turbo::nameof_full_type<decltype(othervar)>() == "Long");
    ASSERT_TRUE(turbo::nameof_full_type<Long>() == "Long");
    ASSERT_TRUE(turbo::nameof_full_type<Long::LL>() == "Long::LL");

    ASSERT_TRUE(turbo::nameof_full_type<Color>() == "Color");
#elif defined(_MSC_VER)
    ASSERT_TRUE(type_name == "struct SomeStruct");
    ASSERT_TRUE(turbo::nameof_full_type<decltype(ptr_s)>() == "struct SomeStruct *");
    ASSERT_TRUE(turbo::nameof_full_type<decltype(ref_s)>() == "struct SomeStruct &");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct>() == "struct SomeStruct");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct *>() == "struct SomeStruct *");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct &>() == "struct SomeStruct &");
    ASSERT_TRUE(turbo::nameof_full_type<const SomeStruct volatile *>() == "struct SomeStruct const volatile *");

    ASSERT_TRUE(turbo::nameof_full_type<SomeClass<int>>() == "class SomeClass<int>");
    ASSERT_TRUE(turbo::nameof_full_type<const SomeClass<int> volatile *>() == "class SomeClass<int> const volatile *");

    ASSERT_TRUE(turbo::nameof_full_type<decltype(othervar)>() == "struct Long");
    ASSERT_TRUE(turbo::nameof_full_type<Long>() == "struct Long");
    ASSERT_TRUE(turbo::nameof_full_type<Long::LL>() == "struct Long::LL");

    ASSERT_TRUE(turbo::nameof_full_type<Color>() == "enum Color");
#elif defined(__GNUC__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_full_type<decltype(ptr_s)>() == "SomeStruct*");
    ASSERT_TRUE(turbo::nameof_full_type<decltype(ref_s)>() == "SomeStruct&");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct *>() == "SomeStruct*");
    ASSERT_TRUE(turbo::nameof_full_type<SomeStruct &>() == "SomeStruct&");
    ASSERT_TRUE(turbo::nameof_full_type<const SomeStruct volatile *>() == "const volatile SomeStruct*");

    ASSERT_TRUE(turbo::nameof_full_type<SomeClass<int>>() == "SomeClass<int>");
    ASSERT_TRUE(turbo::nameof_full_type<const SomeClass<int> volatile *>() == "const volatile SomeClass<int>*");

    ASSERT_TRUE(turbo::nameof_full_type<decltype(othervar)>() == "Long");
    ASSERT_TRUE(turbo::nameof_full_type<Long>() == "Long");
    ASSERT_TRUE(turbo::nameof_full_type<Long::LL>() == "Long::LL");

    ASSERT_TRUE(turbo::nameof_full_type<Color>() == "Color");
#endif
}

TEST(REFLECTION_ENUM, nameof_nameof_short_type) {
    constexpr auto type_name = turbo::nameof_short_type<decltype(struct_var)>();
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_short_type<decltype(ref_s)>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_short_type<SomeStruct>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_short_type<SomeStruct &>() == "SomeStruct");
    ASSERT_TRUE(turbo::nameof_short_type<const SomeStruct volatile>() == "SomeStruct");

    ASSERT_TRUE(turbo::nameof_short_type<SomeClass<int>>() == "SomeClass");
    ASSERT_TRUE(turbo::nameof_short_type<const SomeClass<int> volatile>() == "SomeClass");

    ASSERT_TRUE(turbo::nameof_short_type<decltype(othervar)>() == "Long");
    ASSERT_TRUE(turbo::nameof_short_type<Long>() == "Long");
    ASSERT_TRUE(turbo::nameof_short_type<Long::LL>() == "LL");

    ASSERT_TRUE(turbo::nameof_short_type<Color>() == "Color");
}

TEST(REFLECTION_ENUM, REFLECT_TYPE) {
    constexpr auto type_name = REFLECT_TYPE(decltype(struct_var));
#if defined(__clang__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(decltype(ptr_s)) == "SomeStruct *");
    ASSERT_TRUE(REFLECT_TYPE(decltype(ref_s)) == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(SomeStruct) == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(SomeStruct *) == "SomeStruct *");
    ASSERT_TRUE(REFLECT_TYPE(const SomeStruct &) == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(const SomeStruct volatile *) == "const volatile SomeStruct *");

    ASSERT_TRUE(REFLECT_TYPE(SomeClass<int>) == "SomeClass<int>");
    ASSERT_TRUE(REFLECT_TYPE(const SomeClass<int> volatile *) == "const volatile SomeClass<int> *");

    ASSERT_TRUE(REFLECT_TYPE(decltype(othervar)) == "Long");
    ASSERT_TRUE(REFLECT_TYPE(Long) == "Long");
    ASSERT_TRUE(REFLECT_TYPE(Long::LL) == "Long::LL");

    ASSERT_TRUE(REFLECT_TYPE(Color) == "Color");
#elif defined(_MSC_VER)
    ASSERT_TRUE(type_name == "struct SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(decltype(ptr_s)) == "struct SomeStruct *");
    ASSERT_TRUE(REFLECT_TYPE(decltype(ref_s)) == "struct SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(SomeStruct) == "struct SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(SomeStruct *) == "struct SomeStruct *");
    ASSERT_TRUE(REFLECT_TYPE(const SomeStruct &) == "struct SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(const SomeStruct volatile *) == "struct SomeStruct const volatile *");

    ASSERT_TRUE(REFLECT_TYPE(SomeClass<int>) == "class SomeClass<int>");
    ASSERT_TRUE(REFLECT_TYPE(const SomeClass<int> volatile *) == "class SomeClass<int> const volatile *");

    ASSERT_TRUE(REFLECT_TYPE(decltype(othervar)) == "struct Long");
    ASSERT_TRUE(REFLECT_TYPE(Long) == "struct Long");
    ASSERT_TRUE(REFLECT_TYPE(Long::LL) == "struct Long::LL");

    ASSERT_TRUE(REFLECT_TYPE(Color) == "enum Color");
#elif defined(__GNUC__)
    ASSERT_TRUE(type_name == "SomeStruct") << type_name;
    ASSERT_TRUE(REFLECT_TYPE(decltype(ptr_s)) == "SomeStruct*");
    ASSERT_TRUE(REFLECT_TYPE(decltype(ref_s)) == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(SomeStruct) == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(SomeStruct * ) == "SomeStruct*");
    ASSERT_TRUE(REFLECT_TYPE(const SomeStruct &) == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE(const SomeStruct volatile *) == "const volatile SomeStruct*");

    ASSERT_TRUE(REFLECT_TYPE(SomeClass<int>) == "SomeClass<int>");
    ASSERT_TRUE(REFLECT_TYPE(const SomeClass<int> volatile *) == "const volatile SomeClass<int>*");

    ASSERT_TRUE(REFLECT_TYPE(decltype(othervar)) == "Long");
    ASSERT_TRUE(REFLECT_TYPE(Long) == "Long");
    ASSERT_TRUE(REFLECT_TYPE(Long::LL) == "Long::LL");

    ASSERT_TRUE(REFLECT_TYPE(Color) == "Color");
#endif
}

TEST(REFLECTION_ENUM, REFLECT_TYPE_EXPR) {
    constexpr auto type_name = REFLECT_TYPE_EXPR(struct_var);
#if defined(__clang__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(ptr_s) == "SomeStruct *");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(ref_s) == "SomeStruct");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(ptr_c) == "const volatile SomeClass<int> *");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar) == "Long");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar.ll) == "Long::LL");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar.ll.field) == "int");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(Color::RED) == "Color");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "SomeClass<int>");
#elif defined(_MSC_VER)
    ASSERT_TRUE(type_name == "struct SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(ptr_s) == "struct SomeStruct *");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(ref_s) == "struct SomeStruct");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(ptr_c) == "class SomeClass<int> const volatile *");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar) == "struct Long");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar.ll) == "struct Long::LL");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar.ll.field) == "int");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(Color::RED) == "enum Color");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "class SomeClass<int>");
#elif defined(__GNUC__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(ptr_s) == "SomeStruct*");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(ref_s) == "SomeStruct");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(ptr_c) == "const volatile SomeClass<int>*");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar) == "Long");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar.ll) == "Long::LL");
    ASSERT_TRUE(REFLECT_TYPE_EXPR(othervar.ll.field) == "int");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(Color::RED) == "Color");

    ASSERT_TRUE(REFLECT_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "SomeClass<int>");
#endif
}

TEST(REFLECTION_ENUM, REFLECT_FULL_TYPE) {
    constexpr auto type_name = REFLECT_FULL_TYPE(decltype(struct_var));
#if defined(__clang__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(ptr_s)) == "SomeStruct *");
    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(ref_s)) == "SomeStruct &");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct) == "SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct *) == "SomeStruct *");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct &) == "SomeStruct &");
    ASSERT_TRUE(REFLECT_FULL_TYPE(const SomeStruct volatile *) == "const volatile SomeStruct *");

    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeClass<int>) == "SomeClass<int>");
    ASSERT_TRUE(REFLECT_FULL_TYPE(const SomeClass<int> volatile *) == "const volatile SomeClass<int> *");

    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(othervar)) == "Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE(Long) == "Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE(Long::LL) == "Long::LL");

    ASSERT_TRUE(REFLECT_FULL_TYPE(Color) == "Color");
#elif defined(_MSC_VER)
    ASSERT_TRUE(type_name == "struct SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(ptr_s)) == "struct SomeStruct *");
    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(ref_s)) == "struct SomeStruct &");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct) == "struct SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct *) == "struct SomeStruct *");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct &) == "struct SomeStruct &");
    ASSERT_TRUE(REFLECT_FULL_TYPE(const SomeStruct volatile *) == "struct SomeStruct const volatile *");

    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeClass<int>) == "class SomeClass<int>");
    ASSERT_TRUE(REFLECT_FULL_TYPE(const SomeClass<int> volatile *) == "class SomeClass<int> const volatile *");

    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(othervar)) == "struct Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE(Long) == "struct Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE(Long::LL) == "struct Long::LL");

    ASSERT_TRUE(REFLECT_FULL_TYPE(Color) == "enum Color");
#elif defined(__GNUC__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(ptr_s)) == "SomeStruct*");
    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(ref_s)) == "SomeStruct&");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct) == "SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct * ) == "SomeStruct*");
    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeStruct & ) == "SomeStruct&");
    ASSERT_TRUE(REFLECT_FULL_TYPE(const SomeStruct volatile *) == "const volatile SomeStruct*");

    ASSERT_TRUE(REFLECT_FULL_TYPE(SomeClass<int>) == "SomeClass<int>");
    ASSERT_TRUE(REFLECT_FULL_TYPE(const SomeClass<int> volatile *) == "const volatile SomeClass<int>*");

    ASSERT_TRUE(REFLECT_FULL_TYPE(decltype(othervar)) == "Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE(Long) == "Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE(Long::LL) == "Long::LL");

    ASSERT_TRUE(REFLECT_FULL_TYPE(Color) == "Color");
#endif
}

TEST(REFLECTION_ENUM, REFLECT_FULL_TYPE_EXPR) {
    constexpr auto type_name = REFLECT_FULL_TYPE_EXPR(struct_var);
#if defined(__clang__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ptr_s) == "SomeStruct *");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ref_s) == "SomeStruct &");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ptr_c) == "const volatile SomeClass<int> *");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar) == "Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar.ll) == "Long::LL");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar.ll.field) == "int");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(Color::RED) == "Color");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "const SomeClass<int> &&");
#elif defined(_MSC_VER)
    ASSERT_TRUE(type_name == "struct SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ptr_s) == "struct SomeStruct *");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ref_s) == "struct SomeStruct &");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ptr_c) == "class SomeClass<int> const volatile *");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar) == "struct Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar.ll) == "struct Long::LL");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar.ll.field) == "int");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(Color::RED) == "enum Color");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "class SomeClass<int> const &&");
#elif defined(__GNUC__)
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ptr_s) == "SomeStruct*");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ref_s) == "SomeStruct&");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(ptr_c) == "const volatile SomeClass<int>*");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar) == "Long");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar.ll) == "Long::LL");
    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(othervar.ll.field) == "int");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(Color::RED) == "Color");

    ASSERT_TRUE(REFLECT_FULL_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "const SomeClass<int>&&");
#endif
}

TEST(REFLECTION_ENUM, REFLECT_SHORT_TYPE) {
    constexpr auto type_name = REFLECT_SHORT_TYPE(decltype(struct_var));
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_SHORT_TYPE(decltype(ref_s)) == "SomeStruct");
    ASSERT_TRUE(REFLECT_SHORT_TYPE(SomeStruct) == "SomeStruct");
    ASSERT_TRUE(REFLECT_SHORT_TYPE(SomeStruct & ) == "SomeStruct");
    ASSERT_TRUE(REFLECT_SHORT_TYPE(const SomeStruct volatile) == "SomeStruct");

    ASSERT_TRUE(REFLECT_SHORT_TYPE(SomeClass<int>) == "SomeClass");
    ASSERT_TRUE(REFLECT_SHORT_TYPE(const SomeClass<int> volatile) == "SomeClass");

    ASSERT_TRUE(REFLECT_SHORT_TYPE(decltype(othervar)) == "Long");
    ASSERT_TRUE(REFLECT_SHORT_TYPE(Long) == "Long");
    ASSERT_TRUE(REFLECT_SHORT_TYPE(Long::LL) == "LL");

    ASSERT_TRUE(REFLECT_SHORT_TYPE(Color) == "Color");
}

TEST(REFLECTION_ENUM, REFLECT_SHORT_TYPE_EXPR) {
    constexpr auto type_name = REFLECT_SHORT_TYPE_EXPR(struct_var);
    ASSERT_TRUE(type_name == "SomeStruct");
    ASSERT_TRUE(REFLECT_SHORT_TYPE_EXPR(ref_s) == "SomeStruct");

    ASSERT_TRUE(REFLECT_SHORT_TYPE_EXPR(othervar) == "Long");
    ASSERT_TRUE(REFLECT_SHORT_TYPE_EXPR(othervar.ll) == "LL");
    ASSERT_TRUE(REFLECT_SHORT_TYPE_EXPR(othervar.ll.field) == "int");

    ASSERT_TRUE(REFLECT_SHORT_TYPE_EXPR(Color::RED) == "Color");

    ASSERT_TRUE(REFLECT_SHORT_TYPE_EXPR(std::declval<const SomeClass<int>>()) == "SomeClass");
}

#endif

#if defined(REFLECT_TYPE_RTTI_SUPPORTED) && REFLECT_TYPE_RTTI_SUPPORTED

TEST(REFLECTION_ENUM, REFLECT_TYPE_RTTI) {
    TestRtti::Base *ptr = new TestRtti::Derived();
    const TestRtti::Base &const_ref = *ptr;
    volatile TestRtti::Base &volatile_ref = *ptr;
    volatile const TestRtti::Base &cv_ref = *ptr;
#if defined(__clang__) && !defined(_MSC_VER)
    ASSERT_TRUE(REFLECT_TYPE_RTTI(*ptr) == "TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(const_ref) == "TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(volatile_ref) == "TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(cv_ref) == "TestRtti::Derived");
#elif defined(_MSC_VER)
    ASSERT_TRUE(REFLECT_TYPE_RTTI(*ptr) == "struct TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(const_ref) == "struct TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(volatile_ref) == "struct TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(cv_ref) == "struct TestRtti::Derived");
#elif defined(__GNUC__)
    ASSERT_TRUE(REFLECT_TYPE_RTTI(*ptr) == "TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(const_ref) == "TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(volatile_ref) == "TestRtti::Derived");
    ASSERT_TRUE(REFLECT_TYPE_RTTI(cv_ref) == "TestRtti::Derived");
#endif
}

TEST(REFLECTION_ENUM, REFLECT_FULL_TYPE_RTTI) {
    TestRtti::Base *ptr = new TestRtti::Derived();
    const TestRtti::Base &const_ref = *ptr;
    volatile TestRtti::Base &volatile_ref = *ptr;
    volatile const TestRtti::Base &cv_ref = *ptr;
#if defined(__clang__) && !defined(_MSC_VER)
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(*ptr) == "TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(const_ref) == "const TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(volatile_ref) == "volatile TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(cv_ref) == "volatile const TestRtti::Derived&");
#elif defined(_MSC_VER)
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(*ptr) == "struct TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(const_ref) == "const struct TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(volatile_ref) == "volatile struct TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(cv_ref) == "volatile const struct TestRtti::Derived&");
#elif defined(__GNUC__)
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(*ptr) == "TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(const_ref) == "const TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(volatile_ref) == "volatile TestRtti::Derived&");
    ASSERT_TRUE(REFLECT_FULL_TYPE_RTTI(cv_ref) == "volatile const TestRtti::Derived&");
#endif
}

TEST(REFLECT, NAMEOFSHORT_TYPE_RTTI) {
    TestRtti::Base *ptr = new TestRtti::Derived();
    const TestRtti::Base &const_ref = *ptr;
    volatile TestRtti::Base &volatile_ref = *ptr;
    volatile const TestRtti::Base &cv_ref = *ptr;

    ASSERT_TRUE(REFLECT_SHORT_TYPE_RTTI(*ptr) == "Derived");
    ASSERT_TRUE(REFLECT_SHORT_TYPE_RTTI(const_ref) == "Derived");
    ASSERT_TRUE(REFLECT_SHORT_TYPE_RTTI(volatile_ref) == "Derived");
    ASSERT_TRUE(REFLECT_SHORT_TYPE_RTTI(cv_ref) == "Derived");
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

TEST(REFLECTION_ENUM, REFLECT_MEMBER) {
    ASSERT_TRUE(REFLECT_MEMBER(&SomeStruct::somefield) == "somefield");
    ASSERT_TRUE(REFLECT_MEMBER(&SomeStruct::SomeMethod1) == "SomeMethod1");
    ASSERT_TRUE(REFLECT_MEMBER(&Long::LL::field) == "field");
    constexpr auto member_ptr = &SomeStruct::somefield;
    ASSERT_TRUE(REFLECT_MEMBER(member_ptr) == "somefield");
    ASSERT_TRUE(REFLECT_MEMBER(&StructMemberInitializationUsingNameof::teststringfield) == "teststringfield");
    ASSERT_TRUE(REFLECT_MEMBER(&StructWithNonConstexprDestructor::somefield) == "somefield");
}

TEST(REFLECT, nameof_member) {
    ASSERT_TRUE(turbo::nameof_member<&SomeStruct::somefield>() == "somefield");
    ASSERT_TRUE(turbo::nameof_member<&SomeStruct::SomeMethod1>() == "SomeMethod1");
    ASSERT_TRUE(turbo::nameof_member<&Long::LL::field>() == "field");
    constexpr auto member_ptr = &SomeStruct::somefield;
    ASSERT_TRUE(turbo::nameof_member<member_ptr>() == "somefield");
    ASSERT_TRUE(turbo::nameof_member<&StructMemberInitializationUsingNameof::teststringfield>() == "teststringfield");
    ASSERT_TRUE(turbo::nameof_member<&StructWithNonConstexprDestructor::somefield>() == "somefield");
}

#endif

#if defined(REFLECT_POINTER_SUPPORTED) && REFLECT_POINTER_SUPPORTED

void somefunction() {}

TEST(REFLECTION_ENUM, REFLECT_POINTER) {
    ASSERT_TRUE(REFLECT_POINTER(&SomeStruct::somestaticfield) == "somestaticfield");
    ASSERT_TRUE(REFLECT_POINTER(&SomeStruct::someotherstaticfield) == "someotherstaticfield");
    ASSERT_TRUE(REFLECT_POINTER(static_cast<const char *>(nullptr)) == "nullptr");
    ASSERT_TRUE(REFLECT_POINTER(static_cast<int ***>(nullptr)) == "nullptr");
    constexpr auto global_ptr = &someglobalvariable;
    ASSERT_TRUE(REFLECT_POINTER(global_ptr) == "someglobalvariable");
    ASSERT_TRUE(REFLECT_POINTER(&someglobalconstvariable) == "someglobalconstvariable");
    ASSERT_TRUE(REFLECT_POINTER(&somefunction) == "somefunction");
}

TEST(REFLECTION_ENUM, nameof_pointer) {
    ASSERT_TRUE(turbo::nameof_pointer<&SomeStruct::somestaticfield>() == "somestaticfield");
    ASSERT_TRUE(turbo::nameof_pointer<&SomeStruct::someotherstaticfield>() == "someotherstaticfield");
    ASSERT_TRUE(turbo::nameof_pointer<static_cast<const char *>(nullptr)>() == "nullptr");
    ASSERT_TRUE(turbo::nameof_pointer<static_cast<int ***>(nullptr)>() == "nullptr");
    constexpr auto global_ptr = &someglobalvariable;
    ASSERT_TRUE(turbo::nameof_pointer<global_ptr>() == "someglobalvariable");
    ASSERT_TRUE(turbo::nameof_pointer<&someglobalconstvariable>() == "someglobalconstvariable");
    ASSERT_TRUE(turbo::nameof_pointer<&somefunction>() == "somefunction");
}

#endif
