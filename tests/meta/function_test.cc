// Copyright 2023 The titan-search Authors.
// Copyright 2015-2020 Denis Blank <denis.blank at outlook dot com>
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

#include "function_test.h"

COPYABLE_LEFT_TYPED_TEST_CASE(AllViewTests)

TYPED_TEST(AllViewTests, CallSucceedsIfNonEmpty) {
    typename TestFixture::template left_t<bool()> left = returnTrue;

    {
        typename TestFixture::template left_view_t<bool()> view(left);
        EXPECT_TRUE(view());
    }

    {
        typename TestFixture::template left_view_t<bool()> view;
        view = left;
        EXPECT_TRUE(view());
    }
}

TYPED_TEST(AllViewTests, CallSucceedsOfFunctionPointers) {
    typename TestFixture::template left_view_t<bool()> view(returnTrue);
    EXPECT_TRUE(view());
}

TYPED_TEST(AllViewTests, CallSucceedsIfCopyConstructed) {
    typename TestFixture::template left_t<bool()> left = returnTrue;
    typename TestFixture::template left_view_t<bool()> right(left);
    typename TestFixture::template left_view_t<bool()> view(left);
    EXPECT_TRUE(view());
}

TYPED_TEST(AllViewTests, CallSucceedsIfMoveConstructed) {
    typename TestFixture::template left_t<bool()> left = returnTrue;
    typename TestFixture::template left_view_t<bool()> right(left);
    typename TestFixture::template left_view_t<bool()> view(std::move(left));
    EXPECT_TRUE(view());
}

TYPED_TEST(AllViewTests, CallSucceedsIfCopyAssigned) {
    typename TestFixture::template left_t<bool()> left = returnTrue;
    typename TestFixture::template left_view_t<bool()> right(left);
    typename TestFixture::template left_view_t<bool()> view;
    view = right;
    EXPECT_TRUE(view());
}

TYPED_TEST(AllViewTests, CallSucceedsIfMoveAssigned) {
    typename TestFixture::template left_t<bool()> left = returnTrue;
    typename TestFixture::template left_view_t<bool()> right(left);
    typename TestFixture::template left_view_t<bool()> view;
    view = std::move(right);
    EXPECT_TRUE(view());
}

TYPED_TEST(AllViewTests, EmptyCorrect) {
    {
        typename TestFixture::template left_t<bool()> left = returnTrue;
        typename TestFixture::template left_view_t<bool()> view(left);
        EXPECT_TRUE(bool(view));
    }
    {
        typename TestFixture::template left_view_t<bool()> view;
        EXPECT_FALSE(bool(view));
    }
}

TYPED_TEST(AllViewTests, IsClearable) {
    typename TestFixture::template left_t<bool()> left = returnTrue;
    typename TestFixture::template left_view_t<bool()> view(left);
    EXPECT_TRUE(bool(view));
    EXPECT_TRUE(view());
    view = nullptr;
    EXPECT_FALSE(bool(view));
}

TYPED_TEST(AllViewTests, IsConstCorrect) {
    {
        typename TestFixture::template left_t<bool() const> left = returnTrue;
        typename TestFixture::template left_view_t<bool() const> view(left);
        EXPECT_TRUE(view());
    }

    {
        typename TestFixture::template left_t<bool() const> left = returnTrue;
        typename TestFixture::template left_view_t<bool()> view(left);
        EXPECT_TRUE(view());
    }

    {
        typename TestFixture::template left_view_t<bool() const> view(returnTrue);
        EXPECT_TRUE(view());
    }
}

TYPED_TEST(AllViewTests, IsVolatileCorrect) {
    {
        typename TestFixture::template left_t<bool() volatile> left = returnTrue;
        typename TestFixture::template left_view_t<bool() volatile> view(left);
        EXPECT_TRUE(view());
    }

    {
        typename TestFixture::template left_view_t<bool() volatile> view(
                returnTrue);
        EXPECT_TRUE(view());
    }
}

TYPED_TEST(AllViewTests, HasCorrectObjectSize) {
    typename TestFixture::template left_view_t<bool() volatile> view;
    EXPECT_EQ(sizeof(view), 2 * sizeof(void *));
}


ALL_LEFT_TYPED_TEST_CASE(AllTypeCheckTests)

TYPED_TEST(AllTypeCheckTests, IsDeclareableWithSupportedTypes) {
    {
        typename TestFixture::template left_t<bool()> left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() const> left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() const> const left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() volatile> left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() volatile> volatile left =
                returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() const volatile> left =
                returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<
                bool() const volatile> const volatile left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() &> left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() &> left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() const &> left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() const &> const left =
                returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() volatile &> left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() volatile &> volatile left =
                returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() const volatile &> left =
                returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<
                bool() const volatile &> const volatile left = returnTrue;
        EXPECT_TRUE(left());
    }
    {
        typename TestFixture::template left_t<bool() &&> left = returnTrue;
        EXPECT_TRUE(std::move(left)());
    }
    {
        typename TestFixture::template left_t<bool() const &&> left = returnTrue;
        EXPECT_TRUE(std::move(left)());
    }
    {
        typename TestFixture::template left_t<bool() const &&> const left =
                returnTrue;
        EXPECT_TRUE(std::move(left)());
    }
    {
        typename TestFixture::template left_t<bool() volatile &&> left = returnTrue;
        EXPECT_TRUE(std::move(left)());
    }
    {
        typename TestFixture::template left_t<bool() volatile &&> volatile left =
                returnTrue;
        EXPECT_TRUE(std::move(left)());
    }
    {
        typename TestFixture::template left_t<bool() const volatile &&> left =
                returnTrue;
        EXPECT_TRUE(std::move(left)());
    }
    {
        typename TestFixture::template left_t<
                bool() const volatile &&> const volatile left = returnTrue;
        EXPECT_TRUE(std::move(left)());
    }
}


ALL_LEFT_TYPED_TEST_CASE(StandardCompliantTest)

TYPED_TEST(StandardCompliantTest, IsSwappableWithMemberMethod) {
    // The standard only requires that functions
    // with the same signature are swappable
    typename TestFixture::template left_t<bool()> left = returnTrue;
    typename TestFixture::template left_t<bool()> right = returnFalse;
    EXPECT_TRUE(left());
    EXPECT_FALSE(right());
    left.swap(right);
    EXPECT_TRUE(right());
    EXPECT_FALSE(left());
    right.swap(left);
    EXPECT_TRUE(left());
    EXPECT_FALSE(right());
}

TYPED_TEST(StandardCompliantTest, IsSwappableWithStdSwap) {
    // The standard only requires that functions
    // with the same signature are swappable
    typename TestFixture::template left_t<bool()> left = returnTrue;
    typename TestFixture::template left_t<bool()> right = returnFalse;
    EXPECT_TRUE(left());
    EXPECT_FALSE(right());
    std::swap(left, right);
    EXPECT_TRUE(right());
    EXPECT_FALSE(left());
    std::swap(left, right);
    EXPECT_TRUE(left());
    EXPECT_FALSE(right());
}

TYPED_TEST(StandardCompliantTest, IsSwappableWithSelf) {
    typename TestFixture::template left_t<bool()> left;
    left.swap(left);
    EXPECT_FALSE(left);
    left = returnTrue;
    left.swap(left);
    EXPECT_TRUE(left);
    EXPECT_TRUE(left());
}

TYPED_TEST(StandardCompliantTest, IsAssignableWithMemberMethod) {
    typename TestFixture::template left_t<bool()> left;
    EXPECT_FALSE(left);
    left.assign(returnFalse, std::allocator<int>{});
    EXPECT_FALSE(left());
    left.assign(returnTrue, std::allocator<int>{});
    EXPECT_TRUE(left());
}

TYPED_TEST(StandardCompliantTest, IsCompareableWithNullptrT) {
    typename TestFixture::template left_t<bool()> left;
    EXPECT_TRUE(left == nullptr);
    EXPECT_TRUE(nullptr == left);
    EXPECT_FALSE(left != nullptr);
    EXPECT_FALSE(nullptr != left);
    left = returnFalse;
    EXPECT_FALSE(left == nullptr);
    EXPECT_FALSE(nullptr == left);
    EXPECT_TRUE(left != nullptr);
    EXPECT_TRUE(nullptr != left);
}


struct stateful_callable {
    std::string test;

    void operator()() {
    }
};

/// Iterator dereference (nullptr) crash in Visual Studio
///
/// This was caused through an issue with the allocated pointer swap on move
TEST(regression_tests, move_iterator_dereference_nullptr) {
    std::string test = "hey";
    turbo::function<void()> fn = stateful_callable{std::move(test)};

    auto fn2 = std::move(fn);
    (void) fn2;
}

int function_issue_7_regression(int &i) {
    return i;
}

/// The following code does not compile on
/// MSVC version 19.12.25830.2 (Visual Studio 2017 15.5.1):
///
/// https://github.com/Naios/function2/issues/7
TEST(regression_tests, reference_parameters_issue_7) {
    turbo::function<int(int &)> f = function_issue_7_regression;
    int i = 4384674;
    ASSERT_EQ(f(i), 4384674);
}

struct scalar_member {
    explicit scalar_member(int num) : num_(num) {
    }

    int num_;
};

/// https://github.com/Naios/function2/issues/10
TEST(regression_tests, scalar_members_issue_10) {
    scalar_member const obj(4384674);

    turbo::function<int(scalar_member const &)> fn = &scalar_member::num_;
    ASSERT_EQ(fn(obj), 4384674);
}

TEST(regression_tests, size_match_layout) {
    turbo::function<void() const> fn;

    ASSERT_EQ(sizeof(fn), turbo::detail::object_size::value);
}

struct trash_obj {
    int raw[3];

    int operator()() {
        return 12345;
    }
};

template<typename T>
struct no_allocate_allocator {
    using value_type = T;
    using size_type = size_t;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    no_allocate_allocator() = default;

    template<typename O>
    no_allocate_allocator(no_allocate_allocator<O>) {
    }

    template<typename M>
    struct rebind {
        typedef no_allocate_allocator<M> other;
    };

    pointer allocate(size_type, void const * = nullptr) {
        EXPECT_TRUE(false);
        return nullptr;
    }

    void deallocate(pointer, size_type) {
        FAIL();
    }
};

TEST(regression_tests, can_take_capacity_obj) {
    turbo::function_base<true, true, turbo::capacity_can_hold<trash_obj>, false, true,
            int()>
            fn;

    fn.assign(trash_obj{}, no_allocate_allocator<trash_obj>{});

    ASSERT_EQ(fn(), 12345);
}

static int call(turbo::function_view<int()> fun) {
    return fun();
}

// https://github.com/Naios/function2/issues/13
TEST(regression_tests, can_convert_nonowning_noncopyable_view) {
    turbo::unique_function<int()> fun = []() mutable { return 12345; };
    int result = call(fun);
    ASSERT_EQ(result, 12345);
}

TEST(regression_tests, can_assign_nonowning_noncopyable_view) {
    turbo::unique_function<int()> fun = []() mutable { return 12345; };
    turbo::function_view<int()> fv;
    fv = fun;
    int result = fv();
    ASSERT_EQ(result, 12345);
}

static turbo::unique_function<void()> issue_14_create() {
    // remove the commented dummy capture to be compilable
    turbo::unique_function<void()> func =
            [i = std::vector<std::vector<std::unique_ptr<int>>>{}
                    // ,dummy = std::unique_ptr<int>()
            ]() {
                // ...
            };

    return std::move(func);
}

// https://github.com/Naios/function2/issues/14
TEST(regression_tests, issue_14) {
    issue_14_create()();
}

struct no_strong_except {
    no_strong_except() = default;

    ~no_strong_except() noexcept(false) {
    }

    no_strong_except(no_strong_except &&) noexcept(false) {
    }

    no_strong_except &operator=(no_strong_except &&) noexcept(false) {
        return *this;
    }

    int operator()() const {
        return 23383;
    }
};

static_assert(!std::is_nothrow_move_constructible<no_strong_except>::value, "");
static_assert(!std::is_nothrow_destructible<no_strong_except>::value, "");

// https://github.com/Naios/function2/issues/20
TEST(regression_tests, can_take_no_strong_except) {
    turbo::function_base<true, false, turbo::capacity_none, true, false, int()> fn;

    fn = no_strong_except{};

    ASSERT_EQ(fn(), 23383);
}

// https://github.com/Naios/function2/issues/23
TEST(regression_tests, can_be_stored_in_vector) {
    using fun_t = turbo::unique_function<int(int)>;

    std::vector<fun_t> v;
    v.reserve(1);
    fun_t f{[](int i) { return 2 * i; }};
    fun_t f2{[](int i) { return 2 * i; }};
    v.emplace_back(std::move(f));
    v.emplace_back(std::move(f2));

    auto const res = v[0](7);
    ASSERT_EQ(res, 14);
}

TEST(regression_tests, unique_non_copyable) {
    using fun_t = turbo::unique_function<int(int)>;
    ASSERT_FALSE(std::is_copy_assignable<fun_t>::value);
    ASSERT_FALSE(std::is_copy_constructible<fun_t>::value);
}

// https://github.com/Naios/function2/issues/21
/*TEST(regression_tests, can_bind_const_view) {
  auto const callable = [] { return 5; };

  turbo::function_view<int() const> view(callable);

  ASSERT_EQ(view(), 5);
}*/

// https://github.com/Naios/function2/issues/48
// -Waddress warning generated for non-capturing lambdas on gcc <= 9.2 #48
TEST(regression_tests, no_address_warning_in_constexpr_lambda) {
    using fun_t = turbo::function<int()>;
    fun_t f([] { return 3836474; });

    ASSERT_EQ(f(), 3836474);
}

struct custom_falsy_invocable {
    operator bool() const {
        return false;
    }

    int operator()() const {
        return 0;
    }
};

TEST(regression_tests, custom_falsy_invocable) {
    turbo::function<int()> f(custom_falsy_invocable{});

#if defined(FU2_HAS_LIMITED_EMPTY_PROPAGATION) || \
    defined(FU2_HAS_NO_EMPTY_PROPAGATION)
    ASSERT_TRUE(static_cast<bool>(f));
#else
    ASSERT_FALSE(static_cast<bool>(f));
#endif
}

namespace issue_35 {
    class ref_obj {
    public:
        ref_obj() = default;

        ref_obj(ref_obj const &) = delete;

        ref_obj(ref_obj &&) = default;

        ref_obj &operator=(ref_obj &&) = default;

        ref_obj &operator=(ref_obj const &) = delete;

        int data() const {
            return data_;
        }

    private:
        int data_{8373827};
    };

    ref_obj &ref_obj_getter() {
        static ref_obj some;
        return some;
    }
} // namespace issue_35

ALL_LEFT_TYPED_TEST_CASE(AllReferenceRetConstructTests)

// https://github.com/Naios/function2/issues/35
TYPED_TEST(AllReferenceRetConstructTests, reference_returns_not_buildable) {
    using namespace issue_35;

    typename TestFixture::template left_t<ref_obj &()> left(&ref_obj_getter);
    ref_obj &ref = left();
    ASSERT_EQ(ref.data(), 8373827);
}

ALL_LEFT_TYPED_TEST_CASE(OverloadTests)

template<typename TestFixture>
struct FunctionProvider {
    bool OverloadedMethod(
            typename TestFixture::template left_t<void(std::false_type)>) {
        return false;
    }

    bool OverloadedMethod(
            typename TestFixture::template left_t<void(std::true_type)>) {
        return true;
    }
};

TYPED_TEST(OverloadTests, IsOverloadable) {
    // Test whether turbo::function supports overloading which isn't possible
    // with C++11 std::function implementations because of
    // a non SFINAE guarded templated constructor.
    FunctionProvider<TestFixture> provider;
    int i = 0;
    EXPECT_TRUE(provider.OverloadedMethod([&](std::true_type) { ++i; }));
}


ALL_LEFT_TYPED_TEST_CASE(AllNoExceptTests)

TYPED_TEST(AllNoExceptTests, CallSucceedsIfNonEmpty) {
    typename TestFixture::template left_t<bool(), false> left = returnTrue;
    EXPECT_TRUE(left());
}

TEST(StrongNoExceptTests, GuaranteedNoExceptOperations) {
    using type = turbo::function_base<true, true, turbo::capacity_fixed<100U>, true,
            true, void()>;

    EXPECT_TRUE(std::is_nothrow_destructible<type>::value);
    EXPECT_TRUE(std::is_nothrow_move_assignable<type>::value);
    EXPECT_TRUE(std::is_nothrow_move_constructible<type>::value);
}

// Issue #5
// Death tests are causing issues when doing leak checks in valgrind
#ifndef TESTS_NO_DEATH_TESTS
TYPED_TEST(AllNoExceptTests, CallAbortsIfEmpty) {
    typename TestFixture::template left_t<bool(), false> left;
    EXPECT_DEATH(left(), "");
}

#endif // TESTS_NO_DEATH_TESTS

#ifdef FU2_HAS_CXX17_NOEXCEPT_FUNCTION_TYPE
TYPED_TEST(AllNoExceptTests, NoExceptCallSuceeds) {
    typename TestFixture::template left_t<int() noexcept> left = []() noexcept {
        return 12345;
    };
    ASSERT_EQ(left(), 12345);
}

TYPED_TEST(AllNoExceptTests, NoexceptDontAffectOverloads) {
    using Type = typename TestFixture::template left_t<void() noexcept>;
    struct Storage {
        Storage(Type) {
        }

        Storage(Storage &&) {
        }
    };
    Storage s1{[]() noexcept {}};
    Storage s2{std::move(s1)};
}

#ifndef TESTS_NO_DEATH_TESTS
TYPED_TEST(AllNoExceptTests, CallAbortsIfEmptyAndNoExcept) {
    typename TestFixture::template left_t<bool() noexcept> left;
    EXPECT_DEATH(left(), "");
}

#endif // TESTS_NO_DEATH_TESTS
#endif // FU2_HAS_CXX17_NOEXCEPT_FUNCTION_TYPE


ALL_LEFT_TYPED_TEST_CASE(MultiSignatureTests)

TYPED_TEST(MultiSignatureTests, CanInvokeMultipleSignatures) {
    typename TestFixture::template left_multi_t<bool(std::true_type) const,
            bool(std::false_type) const>
            left = turbo::overload([](std::true_type) { return true; },
                                   [](std::false_type) { return false; });

    EXPECT_TRUE(left(std::true_type{}));
    EXPECT_FALSE(left(std::false_type{}));
}

TYPED_TEST(MultiSignatureTests, CanInvokeGenericSignatures) {
    typename TestFixture::template left_multi_t<bool(std::true_type) const,
            bool(std::false_type) const>
            left = [](auto value) { return bool(value); };

    EXPECT_TRUE(left(std::true_type{}));
    EXPECT_FALSE(left(std::false_type{}));
}


namespace {
/// Increases the linked counter once for every destruction
    class DeallocatorChecker {
        std::shared_ptr<std::reference_wrapper<std::size_t>> checker_;

    public:
        DeallocatorChecker(std::size_t &checker) {
            checker_ = std::shared_ptr<std::reference_wrapper<std::size_t>>(
                    new std::reference_wrapper<std::size_t>(checker),
                    [=](std::reference_wrapper<std::size_t> *ptr) {
                        ++ptr->get();
                        std::default_delete<std::reference_wrapper<std::size_t>>{}(ptr);
                    });
        }

        std::size_t operator()() const {
            return checker_->get();
        }
    };

    struct VolatileProvider {
        bool operator()() volatile {
            return true;
        }
    };

    struct ConstProvider {
        bool operator()() const {
            return true;
        }
    };

    struct ConstVolatileProvider {
        bool operator()() const volatile {
            return true;
        }
    };

    struct RValueProvider {
        bool operator()() &&{
            return true;
        }
    };
} // namespace

ALL_LEFT_TYPED_TEST_CASE(AllSingleMoveAssignConstructTests)

TYPED_TEST(AllSingleMoveAssignConstructTests, AreEmptyOnDefaultConstruct) {
    typename TestFixture::template left_t<bool()> left;
    EXPECT_FALSE(left);
    EXPECT_TRUE(left.empty());
    left = returnTrue;
    EXPECT_FALSE(left.empty());
    EXPECT_TRUE(left);
    EXPECT_TRUE(left());
}

TYPED_TEST(AllSingleMoveAssignConstructTests, AreNonEmptyOnFunctorConstruct) {
    typename TestFixture::template left_t<bool()> left(returnTrue);
    EXPECT_TRUE(left);
    EXPECT_FALSE(left.empty());
    EXPECT_TRUE(left());
}

TYPED_TEST(AllSingleMoveAssignConstructTests, AreEmptyOnNullptrConstruct) {
    typename TestFixture::template left_t<bool()> left(nullptr);
    EXPECT_FALSE(left);
    EXPECT_TRUE(left.empty());
}

TYPED_TEST(AllSingleMoveAssignConstructTests, AreEmptyAfterNullptrAssign) {
    typename TestFixture::template left_t<bool()> left(returnTrue);
    EXPECT_TRUE(left);
    EXPECT_FALSE(left.empty());
    EXPECT_TRUE(left());
    left = nullptr;
    EXPECT_FALSE(left);
    EXPECT_TRUE(left.empty());
}

TYPED_TEST(AllSingleMoveAssignConstructTests,
           AreFreeingResourcesOnDestruction) {

    // Pre test
    {
        std::size_t deallocates = 0UL;

        {
            DeallocatorChecker checker{deallocates};
            ASSERT_EQ(deallocates, 0UL);
        }
        ASSERT_EQ(deallocates, 1UL);
    }

    // Real test
    {
        std::size_t deallocates = 0UL;

        {
            typename TestFixture::template left_t<std::size_t() const> left(
                    DeallocatorChecker{deallocates});
            EXPECT_EQ(deallocates, 0UL);
        }
        EXPECT_EQ(deallocates, 1UL);
    }
}

TYPED_TEST(AllSingleMoveAssignConstructTests, AreConstructibleFromFunctors) {
    bool result = true;
    typename TestFixture::template left_t<bool(bool)> left(
            [=](bool in) { return result && in; });
    EXPECT_TRUE(left);
    EXPECT_FALSE(left.empty());
    EXPECT_TRUE(left(true));
}

TYPED_TEST(AllSingleMoveAssignConstructTests, AreConstructibleFromBind) {
    typename TestFixture::template left_t<bool()> left(
            std::bind(std::logical_and<bool>{}, true, true));
    EXPECT_TRUE(left);
    EXPECT_FALSE(left.empty());
    EXPECT_TRUE(left());
}

/*
TYPED_TEST(AllSingleMoveAssignConstructTests, ProvideItsSignatureAsOperator) {
  EXPECT_TRUE(
      (std::is_same<typename TestFixture::template left_t<void()>::return_type,
                    void>::value));

  EXPECT_TRUE(
      (std::is_same<typename TestFixture::template left_t<float()>::return_type,
                    float>::value));

  EXPECT_TRUE((std::is_same<typename TestFixture::template left_t<void(
                                float, double, int)>::argument_type,
                            std::tuple<float, double, int>>::value));

  EXPECT_TRUE((std::is_same<typename TestFixture::template left_t<
                                std::tuple<int, float>()>::argument_type,
                            std::tuple<>>::value));
}
*/

TYPED_TEST(AllSingleMoveAssignConstructTests, AcceptsItsQualifier) {
    {
        typename TestFixture::template left_t<bool() volatile> left;
        left = VolatileProvider{};
        EXPECT_TRUE(left());
    }

    {
        typename TestFixture::template left_t<bool() const> left;
        left = ConstProvider{};
        EXPECT_TRUE(left());
    }

    {
        typename TestFixture::template left_t<bool() const volatile> left;
        left = ConstVolatileProvider{};
        EXPECT_TRUE(left());
    }

    {
        typename TestFixture::template left_t<bool() &&> left;
        left = RValueProvider{};
        EXPECT_TRUE(std::move(left)());
    }
}

struct MyTestClass {
    bool result = true;

    bool getResult() const {
        return result;
    }
};

TYPED_TEST(AllSingleMoveAssignConstructTests, AcceptClassMethodPointers) {
    /*
    broken
    typename TestFixture::template left_t<bool(MyTestClass*)> left
      = &MyTestClass::getResult;

    MyTestClass my_class;

    EXPECT_TRUE(left(&my_class));*/
}


namespace {
/// Coroutine which increases it's return value by every call
    class UniqueIncreasingCoroutine {
        std::unique_ptr<std::size_t> state = make_unique<std::size_t>(0);

    public:
        UniqueIncreasingCoroutine() {
        }

        std::size_t operator()() {
            return (*state)++;
        }
    };

/// Coroutine which increases it's return value by every call
    class CopyableIncreasingCoroutine {
        std::size_t state = 0UL;

    public:
        CopyableIncreasingCoroutine() {
        }

        std::size_t operator()() {
            return state++;
        }
    };

/// Functor which returns it's shared count
    class SharedCountFunctor {
        std::shared_ptr<std::size_t> state = std::make_shared<std::size_t>(0);

    public:
        std::size_t operator()() const {
            return state.use_count();
        }
    };
} // namespace

ALL_LEFT_RIGHT_TYPED_TEST_CASE(AllMoveAssignConstructTests)

TYPED_TEST(AllMoveAssignConstructTests, AreMoveConstructible) {
    typename TestFixture::template right_t<bool()> right = returnTrue;
    typename TestFixture::template left_t<bool()> left(std::move(right));
    EXPECT_TRUE(left());
}

TYPED_TEST(AllMoveAssignConstructTests, AreMoveAssignable) {
    typename TestFixture::template left_t<bool()> left;
    typename TestFixture::template right_t<bool()> right = returnTrue;
    left = std::move(right);
    EXPECT_TRUE(left());
}

TYPED_TEST(AllMoveAssignConstructTests, TransferStatesOnConstruct) {
    typename TestFixture::template left_t<std::size_t()> left;
    typename TestFixture::template right_t<std::size_t()> right =
            CopyableIncreasingCoroutine();
    EXPECT_EQ(right(), 0UL);
    EXPECT_EQ(right(), 1UL);
    left = std::move(right);
    EXPECT_EQ(left(), 2UL);
    EXPECT_EQ(left(), 3UL);
    EXPECT_EQ(left(), 4UL);
}

TYPED_TEST(AllMoveAssignConstructTests, TransferStatesOnAssign) {
    typename TestFixture::template right_t<std::size_t()> right =
            CopyableIncreasingCoroutine();
    EXPECT_EQ(right(), 0UL);
    EXPECT_EQ(right(), 1UL);
    typename TestFixture::template left_t<std::size_t()> left(std::move(right));
    EXPECT_EQ(left(), 2UL);
    EXPECT_EQ(left(), 3UL);
    EXPECT_EQ(left(), 4UL);
}

TYPED_TEST(AllMoveAssignConstructTests, AreEmptyAfterMoveConstruct) {
    typename TestFixture::template right_t<bool()> right = returnTrue;
    EXPECT_TRUE(right);
    EXPECT_TRUE(right());
    typename TestFixture::template left_t<bool()> left(std::move(right));
    EXPECT_FALSE(right);
    EXPECT_TRUE(left);
    EXPECT_TRUE(left());
}

TYPED_TEST(AllMoveAssignConstructTests, AreEmptyAfterMoveAssign) {
    typename TestFixture::template left_t<bool()> left;
    typename TestFixture::template right_t<bool()> right;
    EXPECT_FALSE(left);
    EXPECT_FALSE(right);
    right = returnTrue;
    EXPECT_TRUE(right);
    EXPECT_TRUE(right());
    left = std::move(right);
    EXPECT_FALSE(right);
    EXPECT_TRUE(left);
    EXPECT_TRUE(left());
}

UNIQUE_LEFT_RIGHT_TYPED_TEST_CASE(UniqueMoveAssignConstructTests)

TYPED_TEST(UniqueMoveAssignConstructTests, TransferStateOnMoveConstruct) {
    {
        typename TestFixture::template right_t<std::size_t()> right =
                UniqueIncreasingCoroutine();
        typename TestFixture::template left_t<std::size_t()> left(std::move(right));
        EXPECT_EQ(left(), 0UL);
    }

    {
        typename TestFixture::template right_t<std::size_t()> right =
                UniqueIncreasingCoroutine();
        EXPECT_EQ(right(), 0UL);
        EXPECT_EQ(right(), 1UL);
        EXPECT_EQ(right(), 2UL);
        typename TestFixture::template left_t<std::size_t()> left(std::move(right));
        EXPECT_EQ(left(), 3UL);
        EXPECT_EQ(left(), 4UL);
    }
}

TYPED_TEST(UniqueMoveAssignConstructTests, TransferStateOnMoveAssign) {
    {
        typename TestFixture::template left_t<std::size_t()> left;
        typename TestFixture::template right_t<std::size_t()> right =
                UniqueIncreasingCoroutine();
        left = std::move(right);
        EXPECT_EQ(left(), 0UL);
    }

    {
        typename TestFixture::template left_t<std::size_t()> left;
        typename TestFixture::template right_t<std::size_t()> right =
                UniqueIncreasingCoroutine();
        EXPECT_EQ(right(), 0UL);
        EXPECT_EQ(right(), 1UL);
        EXPECT_EQ(right(), 2UL);
        left = std::move(right);
        EXPECT_EQ(left(), 3UL);
        EXPECT_EQ(left(), 4UL);
    }
}

COPYABLE_LEFT_RIGHT_TYPED_TEST_CASE(CopyableCopyAssignConstructTests)

TYPED_TEST(CopyableCopyAssignConstructTests, AreCopyConstructible) {
    typename TestFixture::template right_t<bool()> right = returnTrue;
    typename TestFixture::template left_t<bool()> left(right);
    EXPECT_TRUE(left());
    EXPECT_TRUE(left);
    EXPECT_TRUE(right());
    EXPECT_TRUE(right);
}

TYPED_TEST(CopyableCopyAssignConstructTests, AreCopyAssignable) {
    typename TestFixture::template left_t<bool()> left;
    typename TestFixture::template right_t<bool()> right = returnTrue;
    EXPECT_FALSE(left);
    left = right;
    EXPECT_TRUE(left());
    EXPECT_TRUE(left);
    EXPECT_TRUE(right());
    EXPECT_TRUE(right);
}

TYPED_TEST(CopyableCopyAssignConstructTests, CopyStateOnCopyConstruct) {
    {
        typename TestFixture::template right_t<std::size_t()> right =
                CopyableIncreasingCoroutine();
        typename TestFixture::template left_t<std::size_t()> left(right);
        EXPECT_EQ(left(), 0UL);
        EXPECT_EQ(right(), 0UL);
    }

    {
        typename TestFixture::template right_t<std::size_t()> right =
                CopyableIncreasingCoroutine();
        EXPECT_EQ(right(), 0UL);
        EXPECT_EQ(right(), 1UL);
        EXPECT_EQ(right(), 2UL);
        typename TestFixture::template left_t<std::size_t()> left(right);
        EXPECT_EQ(left(), 3UL);
        EXPECT_EQ(right(), 3UL);
        EXPECT_EQ(left(), 4UL);
        EXPECT_EQ(right(), 4UL);
    }
}

TYPED_TEST(CopyableCopyAssignConstructTests, CopyStateOnCopyAssign) {
    {
        typename TestFixture::template left_t<std::size_t()> left;
        typename TestFixture::template right_t<std::size_t()> right =
                CopyableIncreasingCoroutine();
        left = right;
        EXPECT_EQ(left(), 0UL);
        EXPECT_EQ(right(), 0UL);
    }

    {
        typename TestFixture::template left_t<std::size_t()> left;
        typename TestFixture::template right_t<std::size_t()> right =
                CopyableIncreasingCoroutine();
        EXPECT_EQ(right(), 0UL);
        EXPECT_EQ(right(), 1UL);
        EXPECT_EQ(right(), 2UL);
        left = right;
        EXPECT_EQ(left(), 3UL);
        EXPECT_EQ(right(), 3UL);
        EXPECT_EQ(left(), 4UL);
        EXPECT_EQ(right(), 4UL);
    }
}
