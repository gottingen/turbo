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

#include "turbo/meta/type_traits.h"

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "turbo/platform/port.h"
#include "turbo/times/clock.h"
#include "turbo/times/time.h"
#include "gtest/gtest.h"

namespace {

    using ::testing::StaticAssertTypeEq;

    template<class T, class U>
    struct simple_pair {
        T first;
        U second;
    };

    struct Dummy {
    };

    struct ReturnType {
    };

    struct ConvertibleToReturnType {
        operator ReturnType() const;  // NOLINT
    };

// Unique types used as parameter types for testing the detection idiom.
    struct StructA {
    };
    struct StructB {
    };
    struct StructC {
    };

    struct TypeWithBarFunction {
        template<class T,
                std::enable_if_t<std::is_same<T &&, StructA &>::value, int> = 0>
        ReturnType bar(T &&, const StructB &, StructC &&) &&;  // NOLINT
    };

    struct TypeWithBarFunctionAndConvertibleReturnType {
        template<class T,
                std::enable_if_t<std::is_same<T &&, StructA &>::value, int> = 0>
        ConvertibleToReturnType bar(T &&, const StructB &, StructC &&) &&;  // NOLINT
    };

    template<class Class, class... Ts>
    using BarIsCallableImpl =
            decltype(std::declval<Class>().bar(std::declval<Ts>()...));

    template<class Class, class... T>
    using BarIsCallable =
            turbo::type_traits_internal::is_detected<BarIsCallableImpl, Class, T...>;

    template<class Class, class... T>
    using BarIsCallableConv = turbo::type_traits_internal::is_detected_convertible<
            ReturnType, BarIsCallableImpl, Class, T...>;

// NOTE: Test of detail type_traits_internal::is_detected.
    TEST(IsDetectedTest, BasicUsage) {
        EXPECT_TRUE((BarIsCallable<TypeWithBarFunction, StructA &, const StructB &,
                StructC>::value));
        EXPECT_TRUE(
                (BarIsCallable<TypeWithBarFunction, StructA &, StructB &, StructC>::value));
        EXPECT_TRUE(
                (BarIsCallable<TypeWithBarFunction, StructA &, StructB, StructC>::value));

        EXPECT_FALSE((BarIsCallable<int, StructA &, const StructB &, StructC>::value));
        EXPECT_FALSE((BarIsCallable<TypeWithBarFunction &, StructA &, const StructB &,
                StructC>::value));
        EXPECT_FALSE((BarIsCallable<TypeWithBarFunction, StructA, const StructB &,
                StructC>::value));
    }

// NOTE: Test of detail type_traits_internal::is_detected_convertible.
    TEST(IsDetectedConvertibleTest, BasicUsage) {
        EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunction, StructA &, const StructB &,
                StructC>::value));
        EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunction, StructA &, StructB &,
                StructC>::value));
        EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunction, StructA &, StructB,
                StructC>::value));
        EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType,
                StructA &, const StructB &, StructC>::value));
        EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType,
                StructA &, StructB &, StructC>::value));
        EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType,
                StructA &, StructB, StructC>::value));

        EXPECT_FALSE(
                (BarIsCallableConv<int, StructA &, const StructB &, StructC>::value));
        EXPECT_FALSE((BarIsCallableConv<TypeWithBarFunction &, StructA &,
                const StructB &, StructC>::value));
        EXPECT_FALSE((BarIsCallableConv<TypeWithBarFunction, StructA, const StructB &,
                StructC>::value));
        EXPECT_FALSE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType &,
                StructA &, const StructB &, StructC>::value));
        EXPECT_FALSE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType,
                StructA, const StructB &, StructC>::value));
    }

    TEST(VoidTTest, BasicUsage) {
        StaticAssertTypeEq<void, turbo::void_t<Dummy>>();
        StaticAssertTypeEq<void, turbo::void_t<Dummy, Dummy, Dummy>>();
    }


    struct MyTrueType {
        static constexpr bool value = true;
    };

    struct MyFalseType {
        static constexpr bool value = false;
    };


    TEST(NegationTest, BasicBooleanLogic) {
        EXPECT_FALSE(turbo::negation<std::true_type>::value);
        EXPECT_FALSE(turbo::negation<MyTrueType>::value);
        EXPECT_TRUE(turbo::negation<std::false_type>::value);
        EXPECT_TRUE(turbo::negation<MyFalseType>::value);
    }

// all member functions are trivial
    class Trivial {
        int n_;
    };

    struct TrivialDestructor {
        ~TrivialDestructor() = default;
    };

    struct NontrivialDestructor {
        ~NontrivialDestructor() {}
    };

    struct DeletedDestructor {
        ~DeletedDestructor() = delete;
    };

    class TrivialDefaultCtor {
    public:
        TrivialDefaultCtor() = default;

        explicit TrivialDefaultCtor(int n) : n_(n) {}

    private:
        int n_;
    };

    class NontrivialDefaultCtor {
    public:
        NontrivialDefaultCtor() : n_(1) {}

    private:
        int n_;
    };

    class DeletedDefaultCtor {
    public:
        DeletedDefaultCtor() = delete;

        explicit DeletedDefaultCtor(int n) : n_(n) {}

    private:
        int n_;
    };

    class TrivialMoveCtor {
    public:
        explicit TrivialMoveCtor(int n) : n_(n) {}

        TrivialMoveCtor(TrivialMoveCtor &&) = default;

        TrivialMoveCtor &operator=(const TrivialMoveCtor &t) {
            n_ = t.n_;
            return *this;
        }

    private:
        int n_;
    };

    class NontrivialMoveCtor {
    public:
        explicit NontrivialMoveCtor(int n) : n_(n) {}

        NontrivialMoveCtor(NontrivialMoveCtor &&t) noexcept: n_(t.n_) {}

        NontrivialMoveCtor &operator=(const NontrivialMoveCtor &) = default;

    private:
        int n_;
    };

    class TrivialCopyCtor {
    public:
        explicit TrivialCopyCtor(int n) : n_(n) {}

        TrivialCopyCtor(const TrivialCopyCtor &) = default;

        TrivialCopyCtor &operator=(const TrivialCopyCtor &t) {
            n_ = t.n_;
            return *this;
        }

    private:
        int n_;
    };

    class NontrivialCopyCtor {
    public:
        explicit NontrivialCopyCtor(int n) : n_(n) {}

        NontrivialCopyCtor(const NontrivialCopyCtor &t) : n_(t.n_) {}

        NontrivialCopyCtor &operator=(const NontrivialCopyCtor &) = default;

    private:
        int n_;
    };


    TEST(TypeTraitsTest, TestRemoveCVRef) {
        EXPECT_TRUE(
                (std::is_same<typename turbo::remove_cvref<int>::type, int>::value));
        EXPECT_TRUE(
                (std::is_same<typename turbo::remove_cvref<int &>::type, int>::value));
        EXPECT_TRUE(
                (std::is_same<typename turbo::remove_cvref<int &&>::type, int>::value));
        EXPECT_TRUE((
                            std::is_same<typename turbo::remove_cvref<const int &>::type, int>::value));
        EXPECT_TRUE(
                (std::is_same<typename turbo::remove_cvref<int *>::type, int *>::value));
        // Does not remove const in this case.
        EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<const int *>::type,
                const int *>::value));
        EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<int[2]>::type,
                int[2]>::value));
        EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<int (&)[2]>::type,
                int[2]>::value));
        EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<int (&&)[2]>::type,
                int[2]>::value));
        EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<const int[2]>::type,
                int[2]>::value));
        EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<const int (&)[2]>::type,
                int[2]>::value));
        EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<const int (&&)[2]>::type,
                int[2]>::value));
    }


    struct TypeA {
    };
    struct TypeB {
    };
    struct TypeC {
    };
    struct TypeD {
    };

    template<typename T>
    struct Wrap {
    };

    enum class TypeEnum {
        A, B, C, D
    };

    struct GetTypeT {
        template<typename T,
                std::enable_if_t<std::is_same<T, TypeA>::value, int> = 0>
        TypeEnum operator()(Wrap<T>) const {
            return TypeEnum::A;
        }

        template<typename T,
                std::enable_if_t<std::is_same<T, TypeB>::value, int> = 0>
        TypeEnum operator()(Wrap<T>) const {
            return TypeEnum::B;
        }

        template<typename T,
                std::enable_if_t<std::is_same<T, TypeC>::value, int> = 0>
        TypeEnum operator()(Wrap<T>) const {
            return TypeEnum::C;
        }

        // NOTE: TypeD is intentionally not handled
    } constexpr GetType = {};

    TEST(TypeTraitsTest, TestEnableIf) {
        EXPECT_EQ(TypeEnum::A, GetType(Wrap<TypeA>()));
        EXPECT_EQ(TypeEnum::B, GetType(Wrap<TypeB>()));
        EXPECT_EQ(TypeEnum::C, GetType(Wrap<TypeC>()));
    }

    struct GetTypeExtT {
        template<typename T>
        turbo::result_of_t<const GetTypeT &(T)> operator()(T &&arg) const {
            return GetType(std::forward<T>(arg));
        }

        TypeEnum operator()(Wrap<TypeD>) const { return TypeEnum::D; }
    } constexpr GetTypeExt = {};

    TEST(TypeTraitsTest, TestResultOf) {
        EXPECT_EQ(TypeEnum::A, GetTypeExt(Wrap<TypeA>()));
        EXPECT_EQ(TypeEnum::B, GetTypeExt(Wrap<TypeB>()));
        EXPECT_EQ(TypeEnum::C, GetTypeExt(Wrap<TypeC>()));
        EXPECT_EQ(TypeEnum::D, GetTypeExt(Wrap<TypeD>()));
    }

    namespace adl_namespace {

        struct DeletedSwap {
        };

        void swap(DeletedSwap &, DeletedSwap &) = delete;

        struct SpecialNoexceptSwap {
            SpecialNoexceptSwap(SpecialNoexceptSwap &&) {}

            SpecialNoexceptSwap &operator=(SpecialNoexceptSwap &&) { return *this; }

            ~SpecialNoexceptSwap() = default;
        };

        void swap(SpecialNoexceptSwap &, SpecialNoexceptSwap &) noexcept {}

    }  // namespace adl_namespace

    TEST(TypeTraitsTest, IsSwappable) {
        using turbo::type_traits_internal::IsSwappable;
        using turbo::type_traits_internal::StdSwapIsUnconstrained;

        EXPECT_TRUE(IsSwappable<int>::value);

        struct S {
        };
        EXPECT_TRUE(IsSwappable<S>::value);

        struct NoConstruct {
            NoConstruct(NoConstruct &&) = delete;

            NoConstruct &operator=(NoConstruct &&) { return *this; }

            ~NoConstruct() = default;
        };

        EXPECT_EQ(IsSwappable<NoConstruct>::value, StdSwapIsUnconstrained::value);
        struct NoAssign {
            NoAssign(NoAssign &&) {}

            NoAssign &operator=(NoAssign &&) = delete;

            ~NoAssign() = default;
        };

        EXPECT_EQ(IsSwappable<NoAssign>::value, StdSwapIsUnconstrained::value);

        EXPECT_FALSE(IsSwappable<adl_namespace::DeletedSwap>::value);

        EXPECT_TRUE(IsSwappable<adl_namespace::SpecialNoexceptSwap>::value);
    }

    TEST(TypeTraitsTest, IsNothrowSwappable) {
        using turbo::type_traits_internal::IsNothrowSwappable;
        using turbo::type_traits_internal::StdSwapIsUnconstrained;

        EXPECT_TRUE(IsNothrowSwappable<int>::value);

        struct NonNoexceptMoves {
            NonNoexceptMoves(NonNoexceptMoves &&) {}

            NonNoexceptMoves &operator=(NonNoexceptMoves &&) { return *this; }

            ~NonNoexceptMoves() = default;
        };

        EXPECT_FALSE(IsNothrowSwappable<NonNoexceptMoves>::value);

        struct NoConstruct {
            NoConstruct(NoConstruct &&) = delete;

            NoConstruct &operator=(NoConstruct &&) { return *this; }

            ~NoConstruct() = default;
        };

        EXPECT_FALSE(IsNothrowSwappable<NoConstruct>::value);

        struct NoAssign {
            NoAssign(NoAssign &&) {}

            NoAssign &operator=(NoAssign &&) = delete;

            ~NoAssign() = default;
        };

        EXPECT_FALSE(IsNothrowSwappable<NoAssign>::value);

        EXPECT_FALSE(IsNothrowSwappable<adl_namespace::DeletedSwap>::value);

        EXPECT_TRUE(IsNothrowSwappable<adl_namespace::SpecialNoexceptSwap>::value);
    }

    TEST(TrivallyRelocatable, Sanity) {
#if !defined(TURBO_HAVE_ATTRIBUTE_TRIVIAL_ABI) || \
    !TURBO_HAVE_BUILTIN(__is_trivially_relocatable)
        GTEST_SKIP() << "No trivial ABI support.";
#endif

        struct Trivial {
        };
        struct NonTrivial {
            NonTrivial(const NonTrivial &) {}  // NOLINT
        };
        struct TURBO_ATTRIBUTE_TRIVIAL_ABI TrivialAbi {
            TrivialAbi(const TrivialAbi &) {}  // NOLINT
        };
        EXPECT_TRUE(turbo::is_trivially_relocatable<Trivial>::value);
        EXPECT_FALSE(turbo::is_trivially_relocatable<NonTrivial>::value);
        EXPECT_TRUE(turbo::is_trivially_relocatable<TrivialAbi>::value);
    }

#ifdef TURBO_HAVE_CONSTANT_EVALUATED

    constexpr int64_t NegateIfConstantEvaluated(int64_t i) {
      if (turbo::is_constant_evaluated()) {
        return -i;
      } else {
        return i;
      }
    }

#endif  // TURBO_HAVE_CONSTANT_EVALUATED

    TEST(TrivallyRelocatable, is_constant_evaluated) {
#ifdef TURBO_HAVE_CONSTANT_EVALUATED
        constexpr int64_t constant = NegateIfConstantEvaluated(42);
        EXPECT_EQ(constant, -42);

        int64_t now = turbo::ToUnixSeconds(turbo::Now());
        int64_t not_constant = NegateIfConstantEvaluated(now);
        EXPECT_EQ(not_constant, now);

        static int64_t const_init = NegateIfConstantEvaluated(42);
        EXPECT_EQ(const_init, -42);
#else
        GTEST_SKIP() << "turbo::is_constant_evaluated is not defined";
#endif  // TURBO_HAVE_CONSTANT_EVALUATED
    }


}  // namespace
