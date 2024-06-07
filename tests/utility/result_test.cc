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

#include <turbo/utility/result_impl.h>

#include <array>
#include <cstddef>
#include <initializer_list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/casts.h>
#include <turbo/memory/memory.h>
#include <turbo/utility/status_impl.h>
#include <tests/status/status_matchers_api.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/any.h>
#include <turbo/types/variant.h>
#include <turbo/meta/utility.h>

namespace {

    using ::turbo_testing::IsOk;
    using ::turbo_testing::IsOkAndHolds;
    using ::testing::AllOf;
    using ::testing::AnyOf;
    using ::testing::AnyWith;
    using ::testing::ElementsAre;
    using ::testing::EndsWith;
    using ::testing::Field;
    using ::testing::HasSubstr;
    using ::testing::Ne;
    using ::testing::Not;
    using ::testing::Pointee;
    using ::testing::StartsWith;
    using ::testing::VariantWith;

    struct CopyDetector {
        CopyDetector() = default;

        explicit CopyDetector(int xx) : x(xx) {}

        CopyDetector(CopyDetector &&d) noexcept
                : x(d.x), copied(false), moved(true) {}

        CopyDetector(const CopyDetector &d) : x(d.x), copied(true), moved(false) {}

        CopyDetector &operator=(const CopyDetector &c) {
            x = c.x;
            copied = true;
            moved = false;
            return *this;
        }

        CopyDetector &operator=(CopyDetector &&c) noexcept {
            x = c.x;
            copied = false;
            moved = true;
            return *this;
        }

        int x = 0;
        bool copied = false;
        bool moved = false;
    };

    testing::Matcher<const CopyDetector &> CopyDetectorHas(int a, bool b, bool c) {
        return AllOf(Field(&CopyDetector::x, a), Field(&CopyDetector::moved, b),
                     Field(&CopyDetector::copied, c));
    }

    class Base1 {
    public:
        virtual ~Base1() {}

        int pad;
    };

    class Base2 {
    public:
        virtual ~Base2() {}

        int yetotherpad;
    };

    class Derived : public Base1, public Base2 {
    public:
        virtual ~Derived() {}

        int evenmorepad;
    };

    class CopyNoAssign {
    public:
        explicit CopyNoAssign(int value) : foo(value) {}

        CopyNoAssign(const CopyNoAssign &other) : foo(other.foo) {}

        int foo;

    private:
        const CopyNoAssign &operator=(const CopyNoAssign &);
    };

    turbo::Result<std::unique_ptr<int>> ReturnUniquePtr() {
        // Uses implicit constructor from T&&
        return turbo::make_unique<int>(0);
    }

    TEST(Result, ElementType) {
        static_assert(std::is_same<turbo::Result<int>::value_type, int>(), "");
        static_assert(std::is_same<turbo::Result<char>::value_type, char>(), "");
    }

    TEST(Result, TestMoveOnlyInitialization) {
        turbo::Result<std::unique_ptr<int>> thing(ReturnUniquePtr());
        ASSERT_TRUE(thing.ok());
        EXPECT_EQ(0, **thing);
        int *previous = thing->get();

        thing = ReturnUniquePtr();
        EXPECT_TRUE(thing.ok());
        EXPECT_EQ(0, **thing);
        EXPECT_NE(previous, thing->get());
    }

    TEST(Result, TestMoveOnlyValueExtraction) {
        turbo::Result<std::unique_ptr<int>> thing(ReturnUniquePtr());
        ASSERT_TRUE(thing.ok());
        std::unique_ptr<int> ptr = *std::move(thing);
        EXPECT_EQ(0, *ptr);

        thing = std::move(ptr);
        ptr = std::move(*thing);
        EXPECT_EQ(0, *ptr);
    }

    TEST(Result, TestMoveOnlyInitializationFromTemporaryByValueOrDie) {
        std::unique_ptr<int> ptr(*ReturnUniquePtr());
        EXPECT_EQ(0, *ptr);
    }

    TEST(Result, TestValueOrDieOverloadForConstTemporary) {
        static_assert(
                std::is_same<
                        const int &&,
                        decltype(std::declval<const turbo::Result<int> &&>().value())>(),
                "value() for const temporaries should return const T&&");
    }

    TEST(Result, TestMoveOnlyConversion) {
        turbo::Result<std::unique_ptr<const int>> const_thing(ReturnUniquePtr());
        EXPECT_TRUE(const_thing.ok());
        EXPECT_EQ(0, **const_thing);

        // Test rvalue converting assignment
        const int *const_previous = const_thing->get();
        const_thing = ReturnUniquePtr();
        EXPECT_TRUE(const_thing.ok());
        EXPECT_EQ(0, **const_thing);
        EXPECT_NE(const_previous, const_thing->get());
    }

    TEST(Result, TestMoveOnlyVector) {
        // Sanity check that turbo::Result<MoveOnly> works in vector.
        std::vector<turbo::Result<std::unique_ptr<int>>> vec;
        vec.push_back(ReturnUniquePtr());
        vec.resize(2);
        auto another_vec = std::move(vec);
        EXPECT_EQ(0, **another_vec[0]);
        EXPECT_EQ(turbo::unknown_error(""), another_vec[1].status());
    }

    TEST(Result, TestDefaultCtor) {
        turbo::Result<int> thing;
        EXPECT_FALSE(thing.ok());
        EXPECT_EQ(thing.status().code(), turbo::StatusCode::kUnknown);
    }

    TEST(Result, StatusCtorForwards) {
        turbo::Status status(turbo::StatusCode::kInternal, "Some error");

        EXPECT_EQ(turbo::Result<int>(status).status().message(), "Some error");
        EXPECT_EQ(status.message(), "Some error");

        EXPECT_EQ(turbo::Result<int>(std::move(status)).status().message(),
                  "Some error");
        EXPECT_NE(status.message(), "Some error");
    }

    TEST(BadStatusOrAccessTest, CopyConstructionWhatOk) {
        turbo::Status error =
                turbo::internal_error("some arbitrary message too big for the sso buffer");
        turbo::BadResultAccess e1{error};
        turbo::BadResultAccess e2{e1};
        EXPECT_THAT(e1.what(), HasSubstr(error.to_string()));
        EXPECT_THAT(e2.what(), HasSubstr(error.to_string()));
    }

    TEST(BadStatusOrAccessTest, CopyAssignmentWhatOk) {
        turbo::Status error =
                turbo::internal_error("some arbitrary message too big for the sso buffer");
        turbo::BadResultAccess e1{error};
        turbo::BadResultAccess e2{turbo::internal_error("other")};
        e2 = e1;
        EXPECT_THAT(e1.what(), HasSubstr(error.to_string()));
        EXPECT_THAT(e2.what(), HasSubstr(error.to_string()));
    }

    TEST(BadStatusOrAccessTest, MoveConstructionWhatOk) {
        turbo::Status error =
                turbo::internal_error("some arbitrary message too big for the sso buffer");
        turbo::BadResultAccess e1{error};
        turbo::BadResultAccess e2{std::move(e1)};
        EXPECT_THAT(e2.what(), HasSubstr(error.to_string()));
    }

    TEST(BadStatusOrAccessTest, MoveAssignmentWhatOk) {
        turbo::Status error =
                turbo::internal_error("some arbitrary message too big for the sso buffer");
        turbo::BadResultAccess e1{error};
        turbo::BadResultAccess e2{turbo::internal_error("other")};
        e2 = std::move(e1);
        EXPECT_THAT(e2.what(), HasSubstr(error.to_string()));
    }

// Define `EXPECT_DEATH_OR_THROW` to test the behavior of `Result::value`,
// which either throws `BadResultAccess` or `LOG(FATAL)` based on whether
// exceptions are enabled.
#ifdef TURBO_HAVE_EXCEPTIONS
#define EXPECT_DEATH_OR_THROW(statement, status_)                  \
  EXPECT_THROW(                                                    \
      {                                                            \
        try {                                                      \
          statement;                                               \
        } catch (const turbo::BadResultAccess& e) {               \
          EXPECT_EQ(e.status(), status_);                          \
          EXPECT_THAT(e.what(), HasSubstr(e.status().to_string())); \
          throw;                                                   \
        }                                                          \
      },                                                           \
      turbo::BadResultAccess);
#else  // TURBO_HAVE_EXCEPTIONS
#define EXPECT_DEATH_OR_THROW(statement, status) \
  EXPECT_DEATH_IF_SUPPORTED(statement, status.ToString());
#endif  // TURBO_HAVE_EXCEPTIONS

    TEST(StatusOrDeathTest, TestDefaultCtorValue) {
        turbo::Result<int> thing;
        EXPECT_DEATH_OR_THROW(thing.value(), turbo::unknown_error(""));
        const turbo::Result<int> thing2;
        EXPECT_DEATH_OR_THROW(thing2.value(), turbo::unknown_error(""));
    }

    TEST(StatusOrDeathTest, TestValueNotOk) {
        turbo::Result<int> thing(turbo::cancelled_error());
        EXPECT_DEATH_OR_THROW(thing.value(), turbo::cancelled_error());
    }

    TEST(StatusOrDeathTest, TestValueNotOkConst) {
        const turbo::Result<int> thing(turbo::unknown_error(""));
        EXPECT_DEATH_OR_THROW(thing.value(), turbo::unknown_error(""));
    }

    TEST(StatusOrDeathTest, TestPointerDefaultCtorValue) {
        turbo::Result<int *> thing;
        EXPECT_DEATH_OR_THROW(thing.value(), turbo::unknown_error(""));
    }

    TEST(StatusOrDeathTest, TestPointerValueNotOk) {
        turbo::Result<int *> thing(turbo::cancelled_error());
        EXPECT_DEATH_OR_THROW(thing.value(), turbo::cancelled_error());
    }

    TEST(StatusOrDeathTest, TestPointerValueNotOkConst) {
        const turbo::Result<int *> thing(turbo::cancelled_error());
        EXPECT_DEATH_OR_THROW(thing.value(), turbo::cancelled_error());
    }

#if GTEST_HAS_DEATH_TEST
    TEST(StatusOrDeathTest, TestStatusCtorStatusOk) {
        EXPECT_DEBUG_DEATH(
                {
                    // This will DCHECK
                    turbo::Result<int> thing(turbo::OkStatus());
                    // In optimized mode, we are actually going to get error::INTERNAL for
                    // status here, rather than crashing, so check that.
                    EXPECT_FALSE(thing.ok());
                    EXPECT_EQ(thing.status().code(), turbo::StatusCode::kInternal);
                },
                "An OK status is not a valid constructor argument");
    }

    TEST(StatusOrDeathTest, TestPointerStatusCtorStatusOk) {
        EXPECT_DEBUG_DEATH(
                {
                    turbo::Result<int *> thing(turbo::OkStatus());
                    // In optimized mode, we are actually going to get error::INTERNAL for
                    // status here, rather than crashing, so check that.
                    EXPECT_FALSE(thing.ok());
                    EXPECT_EQ(thing.status().code(), turbo::StatusCode::kInternal);
                },
                "An OK status is not a valid constructor argument");
    }

#endif

    TEST(Result, ValueAccessor) {
        const int kIntValue = 110;
        {
            turbo::Result<int> status_or(kIntValue);
            EXPECT_EQ(kIntValue, status_or.value());
            EXPECT_EQ(kIntValue, std::move(status_or).value());
        }
        {
            turbo::Result<CopyDetector> status_or(kIntValue);
            EXPECT_THAT(status_or,
                        IsOkAndHolds(CopyDetectorHas(kIntValue, false, false)));
            CopyDetector copy_detector = status_or.value();
            EXPECT_THAT(copy_detector, CopyDetectorHas(kIntValue, false, true));
            copy_detector = std::move(status_or).value();
            EXPECT_THAT(copy_detector, CopyDetectorHas(kIntValue, true, false));
        }
    }

    TEST(Result, BadValueAccess) {
        const turbo::Status kError = turbo::cancelled_error("message");
        turbo::Result<int> status_or(kError);
        EXPECT_DEATH_OR_THROW(status_or.value(), kError);
    }

    TEST(Result, TestStatusCtor) {
        turbo::Result<int> thing(turbo::cancelled_error());
        EXPECT_FALSE(thing.ok());
        EXPECT_EQ(thing.status().code(), turbo::StatusCode::kCancelled);
    }

    TEST(Result, TestValueCtor) {
        const int kI = 4;
        const turbo::Result<int> thing(kI);
        EXPECT_TRUE(thing.ok());
        EXPECT_EQ(kI, *thing);
    }

    struct Foo {
        const int x;

        explicit Foo(int y) : x(y) {}
    };

    TEST(Result, InPlaceConstruction) {
        EXPECT_THAT(turbo::Result<Foo>(turbo::in_place, 10),
                    IsOkAndHolds(Field(&Foo::x, 10)));
    }

    struct InPlaceHelper {
        InPlaceHelper(std::initializer_list<int> xs, std::unique_ptr<int> yy)
                : x(xs), y(std::move(yy)) {}

        const std::vector<int> x;
        std::unique_ptr<int> y;
    };

    TEST(Result, InPlaceInitListConstruction) {
        turbo::Result<InPlaceHelper> status_or(turbo::in_place, {10, 11, 12},
                                                turbo::make_unique<int>(13));
        EXPECT_THAT(status_or, IsOkAndHolds(AllOf(
                Field(&InPlaceHelper::x, ElementsAre(10, 11, 12)),
                Field(&InPlaceHelper::y, Pointee(13)))));
    }

    TEST(Result, Emplace) {
        turbo::Result<Foo> status_or_foo(10);
        status_or_foo.emplace(20);
        EXPECT_THAT(status_or_foo, IsOkAndHolds(Field(&Foo::x, 20)));
        status_or_foo = turbo::invalid_argument_error("msg");
        EXPECT_FALSE(status_or_foo.ok());
        EXPECT_EQ(status_or_foo.status().code(), turbo::StatusCode::kInvalidArgument);
        EXPECT_EQ(status_or_foo.status().message(), "msg");
        status_or_foo.emplace(20);
        EXPECT_THAT(status_or_foo, IsOkAndHolds(Field(&Foo::x, 20)));
    }

    TEST(Result, EmplaceInitializerList) {
        turbo::Result<InPlaceHelper> status_or(turbo::in_place, {10, 11, 12},
                                                turbo::make_unique<int>(13));
        status_or.emplace({1, 2, 3}, turbo::make_unique<int>(4));
        EXPECT_THAT(status_or,
                    IsOkAndHolds(AllOf(Field(&InPlaceHelper::x, ElementsAre(1, 2, 3)),
                                       Field(&InPlaceHelper::y, Pointee(4)))));
        status_or = turbo::invalid_argument_error("msg");
        EXPECT_FALSE(status_or.ok());
        EXPECT_EQ(status_or.status().code(), turbo::StatusCode::kInvalidArgument);
        EXPECT_EQ(status_or.status().message(), "msg");
        status_or.emplace({1, 2, 3}, turbo::make_unique<int>(4));
        EXPECT_THAT(status_or,
                    IsOkAndHolds(AllOf(Field(&InPlaceHelper::x, ElementsAre(1, 2, 3)),
                                       Field(&InPlaceHelper::y, Pointee(4)))));
    }

    TEST(Result, TestCopyCtorStatusOk) {
        const int kI = 4;
        const turbo::Result<int> original(kI);
        const turbo::Result<int> copy(original);
        EXPECT_THAT(copy.status(), IsOk());
        EXPECT_EQ(*original, *copy);
    }

    TEST(Result, TestCopyCtorStatusNotOk) {
        turbo::Result<int> original(turbo::cancelled_error());
        turbo::Result<int> copy(original);
        EXPECT_EQ(copy.status().code(), turbo::StatusCode::kCancelled);
    }

    TEST(Result, TestCopyCtorNonAssignable) {
        const int kI = 4;
        CopyNoAssign value(kI);
        turbo::Result<CopyNoAssign> original(value);
        turbo::Result<CopyNoAssign> copy(original);
        EXPECT_THAT(copy.status(), IsOk());
        EXPECT_EQ(original->foo, copy->foo);
    }

    TEST(Result, TestCopyCtorStatusOKConverting) {
        const int kI = 4;
        turbo::Result<int> original(kI);
        turbo::Result<double> copy(original);
        EXPECT_THAT(copy.status(), IsOk());
        EXPECT_DOUBLE_EQ(*original, *copy);
    }

    TEST(Result, TestCopyCtorStatusNotOkConverting) {
        turbo::Result<int> original(turbo::cancelled_error());
        turbo::Result<double> copy(original);
        EXPECT_EQ(copy.status(), original.status());
    }

    TEST(Result, TestAssignmentStatusOk) {
        // Copy assignmment
        {
            const auto p = std::make_shared<int>(17);
            turbo::Result<std::shared_ptr<int>> source(p);

            turbo::Result<std::shared_ptr<int>> target;
            target = source;

            ASSERT_TRUE(target.ok());
            EXPECT_THAT(target.status(), IsOk());
            EXPECT_EQ(p, *target);

            ASSERT_TRUE(source.ok());
            EXPECT_THAT(source.status(), IsOk());
            EXPECT_EQ(p, *source);
        }

        // Move asssignment
        {
            const auto p = std::make_shared<int>(17);
            turbo::Result<std::shared_ptr<int>> source(p);

            turbo::Result<std::shared_ptr<int>> target;
            target = std::move(source);

            ASSERT_TRUE(target.ok());
            EXPECT_THAT(target.status(), IsOk());
            EXPECT_EQ(p, *target);

            ASSERT_TRUE(source.ok());
            EXPECT_THAT(source.status(), IsOk());
            EXPECT_EQ(nullptr, *source);
        }
    }

    TEST(Result, TestAssignmentStatusNotOk) {
        // Copy assignment
        {
            const turbo::Status expected = turbo::cancelled_error();
            turbo::Result<int> source(expected);

            turbo::Result<int> target;
            target = source;

            EXPECT_FALSE(target.ok());
            EXPECT_EQ(expected, target.status());

            EXPECT_FALSE(source.ok());
            EXPECT_EQ(expected, source.status());
        }

        // Move assignment
        {
            const turbo::Status expected = turbo::cancelled_error();
            turbo::Result<int> source(expected);

            turbo::Result<int> target;
            target = std::move(source);

            EXPECT_FALSE(target.ok());
            EXPECT_EQ(expected, target.status());

            EXPECT_FALSE(source.ok());
            EXPECT_EQ(source.status().code(), turbo::StatusCode::kInternal);
        }
    }

    TEST(Result, TestAssignmentStatusOKConverting) {
        // Copy assignment
        {
            const int kI = 4;
            turbo::Result<int> source(kI);

            turbo::Result<double> target;
            target = source;

            ASSERT_TRUE(target.ok());
            EXPECT_THAT(target.status(), IsOk());
            EXPECT_DOUBLE_EQ(kI, *target);

            ASSERT_TRUE(source.ok());
            EXPECT_THAT(source.status(), IsOk());
            EXPECT_DOUBLE_EQ(kI, *source);
        }

        // Move assignment
        {
            const auto p = new int(17);
            turbo::Result<std::unique_ptr<int>> source(turbo::WrapUnique(p));

            turbo::Result<std::shared_ptr<int>> target;
            target = std::move(source);

            ASSERT_TRUE(target.ok());
            EXPECT_THAT(target.status(), IsOk());
            EXPECT_EQ(p, target->get());

            ASSERT_TRUE(source.ok());
            EXPECT_THAT(source.status(), IsOk());
            EXPECT_EQ(nullptr, source->get());
        }
    }

    struct A {
        int x;
    };

    struct ImplicitConstructibleFromA {
        int x;
        bool moved;

        ImplicitConstructibleFromA(const A &a)  // NOLINT
                : x(a.x), moved(false) {}

        ImplicitConstructibleFromA(A &&a)  // NOLINT
                : x(a.x), moved(true) {}
    };

    TEST(Result, ImplicitConvertingConstructor) {
        EXPECT_THAT(
                turbo::implicit_cast<turbo::Result<ImplicitConstructibleFromA>>(
                        turbo::Result<A>(A{11})),
                IsOkAndHolds(AllOf(Field(&ImplicitConstructibleFromA::x, 11),
                                   Field(&ImplicitConstructibleFromA::moved, true))));
        turbo::Result<A> a(A{12});
        EXPECT_THAT(
                turbo::implicit_cast<turbo::Result<ImplicitConstructibleFromA>>(a),
                IsOkAndHolds(AllOf(Field(&ImplicitConstructibleFromA::x, 12),
                                   Field(&ImplicitConstructibleFromA::moved, false))));
    }

    struct ExplicitConstructibleFromA {
        int x;
        bool moved;

        explicit ExplicitConstructibleFromA(const A &a) : x(a.x), moved(false) {}

        explicit ExplicitConstructibleFromA(A &&a) : x(a.x), moved(true) {}
    };

    TEST(Result, ExplicitConvertingConstructor) {
        EXPECT_FALSE(
                (std::is_convertible<const turbo::Result<A> &,
                        turbo::Result<ExplicitConstructibleFromA>>::value));
        EXPECT_FALSE(
                (std::is_convertible<turbo::Result<A> &&,
                        turbo::Result<ExplicitConstructibleFromA>>::value));
        EXPECT_THAT(
                turbo::Result<ExplicitConstructibleFromA>(turbo::Result<A>(A{11})),
                IsOkAndHolds(AllOf(Field(&ExplicitConstructibleFromA::x, 11),
                                   Field(&ExplicitConstructibleFromA::moved, true))));
        turbo::Result<A> a(A{12});
        EXPECT_THAT(
                turbo::Result<ExplicitConstructibleFromA>(a),
                IsOkAndHolds(AllOf(Field(&ExplicitConstructibleFromA::x, 12),
                                   Field(&ExplicitConstructibleFromA::moved, false))));
    }

    struct ImplicitConstructibleFromBool {
        ImplicitConstructibleFromBool(bool y) : x(y) {}  // NOLINT
        bool x = false;
    };

    struct ConvertibleToBool {
        explicit ConvertibleToBool(bool y) : x(y) {}

        operator bool() const { return x; }  // NOLINT
        bool x = false;
    };

    TEST(Result, ImplicitBooleanConstructionWithImplicitCasts) {
        EXPECT_THAT(turbo::Result<bool>(turbo::Result<ConvertibleToBool>(true)),
                    IsOkAndHolds(true));
        EXPECT_THAT(turbo::Result<bool>(turbo::Result<ConvertibleToBool>(false)),
                    IsOkAndHolds(false));
        EXPECT_THAT(
                turbo::implicit_cast<turbo::Result<ImplicitConstructibleFromBool>>(
                        turbo::Result<bool>(false)),
                IsOkAndHolds(Field(&ImplicitConstructibleFromBool::x, false)));
        EXPECT_FALSE((std::is_convertible<
                turbo::Result<ConvertibleToBool>,
                turbo::Result<ImplicitConstructibleFromBool>>::value));
    }

    TEST(Result, BooleanConstructionWithImplicitCasts) {
        EXPECT_THAT(turbo::Result<bool>(turbo::Result<ConvertibleToBool>(true)),
                    IsOkAndHolds(true));
        EXPECT_THAT(turbo::Result<bool>(turbo::Result<ConvertibleToBool>(false)),
                    IsOkAndHolds(false));
        EXPECT_THAT(
                turbo::Result<ImplicitConstructibleFromBool>{
                        turbo::Result<bool>(false)},
                IsOkAndHolds(Field(&ImplicitConstructibleFromBool::x, false)));
        EXPECT_THAT(
                turbo::Result<ImplicitConstructibleFromBool>{
                        turbo::Result<bool>(turbo::invalid_argument_error(""))},
                Not(IsOk()));

        EXPECT_THAT(
                turbo::Result<ImplicitConstructibleFromBool>{
                        turbo::Result<ConvertibleToBool>(ConvertibleToBool{false})},
                IsOkAndHolds(Field(&ImplicitConstructibleFromBool::x, false)));
        EXPECT_THAT(
                turbo::Result<ImplicitConstructibleFromBool>{
                        turbo::Result<ConvertibleToBool>(turbo::invalid_argument_error(""))},
                Not(IsOk()));
    }

    TEST(Result, ConstImplicitCast) {
        EXPECT_THAT(turbo::implicit_cast<turbo::Result<bool>>(
                turbo::Result<const bool>(true)),
                    IsOkAndHolds(true));
        EXPECT_THAT(turbo::implicit_cast<turbo::Result<bool>>(
                turbo::Result<const bool>(false)),
                    IsOkAndHolds(false));
        EXPECT_THAT(turbo::implicit_cast<turbo::Result<const bool>>(
                turbo::Result<bool>(true)),
                    IsOkAndHolds(true));
        EXPECT_THAT(turbo::implicit_cast<turbo::Result<const bool>>(
                turbo::Result<bool>(false)),
                    IsOkAndHolds(false));
        EXPECT_THAT(turbo::implicit_cast<turbo::Result<const std::string>>(
                turbo::Result<std::string>("foo")),
                    IsOkAndHolds("foo"));
        EXPECT_THAT(turbo::implicit_cast<turbo::Result<std::string>>(
                turbo::Result<const std::string>("foo")),
                    IsOkAndHolds("foo"));
        EXPECT_THAT(
                turbo::implicit_cast<turbo::Result<std::shared_ptr<const std::string>>>(
                        turbo::Result<std::shared_ptr<std::string>>(
                                std::make_shared<std::string>("foo"))),
                IsOkAndHolds(Pointee(std::string("foo"))));
    }

    TEST(Result, ConstExplicitConstruction) {
        EXPECT_THAT(turbo::Result<bool>(turbo::Result<const bool>(true)),
                    IsOkAndHolds(true));
        EXPECT_THAT(turbo::Result<bool>(turbo::Result<const bool>(false)),
                    IsOkAndHolds(false));
        EXPECT_THAT(turbo::Result<const bool>(turbo::Result<bool>(true)),
                    IsOkAndHolds(true));
        EXPECT_THAT(turbo::Result<const bool>(turbo::Result<bool>(false)),
                    IsOkAndHolds(false));
    }

    struct ExplicitConstructibleFromInt {
        int x;

        explicit ExplicitConstructibleFromInt(int y) : x(y) {}
    };

    TEST(Result, ExplicitConstruction) {
        EXPECT_THAT(turbo::Result<ExplicitConstructibleFromInt>(10),
                    IsOkAndHolds(Field(&ExplicitConstructibleFromInt::x, 10)));
    }

    TEST(Result, ImplicitConstruction) {
        // Check implicit casting works.
        auto status_or =
                turbo::implicit_cast<turbo::Result<turbo::variant<int, std::string>>>(10);
        EXPECT_THAT(status_or, IsOkAndHolds(VariantWith<int>(10)));
    }

    TEST(Result, ImplicitConstructionFromInitliazerList) {
        // Note: dropping the explicit std::initializer_list<int> is not supported
        // by turbo::Result or turbo::optional.
        auto status_or =
                turbo::implicit_cast<turbo::Result<std::vector<int>>>({{10, 20, 30}});
        EXPECT_THAT(status_or, IsOkAndHolds(ElementsAre(10, 20, 30)));
    }

    TEST(Result, UniquePtrImplicitConstruction) {
        auto status_or = turbo::implicit_cast<turbo::Result<std::unique_ptr<Base1>>>(
                turbo::make_unique<Derived>());
        EXPECT_THAT(status_or, IsOkAndHolds(Ne(nullptr)));
    }

    TEST(Result, NestedStatusOrCopyAndMoveConstructorTests) {
        turbo::Result<turbo::Result<CopyDetector>> status_or = CopyDetector(10);
        turbo::Result<turbo::Result<CopyDetector>> status_error =
                turbo::invalid_argument_error("foo");
        EXPECT_THAT(status_or,
                    IsOkAndHolds(IsOkAndHolds(CopyDetectorHas(10, true, false))));
        turbo::Result<turbo::Result<CopyDetector>> a = status_or;
        EXPECT_THAT(a, IsOkAndHolds(IsOkAndHolds(CopyDetectorHas(10, false, true))));
        turbo::Result<turbo::Result<CopyDetector>> a_err = status_error;
        EXPECT_THAT(a_err, Not(IsOk()));

        const turbo::Result<turbo::Result<CopyDetector>> &cref = status_or;
        turbo::Result<turbo::Result<CopyDetector>> b = cref;  // NOLINT
        EXPECT_THAT(b, IsOkAndHolds(IsOkAndHolds(CopyDetectorHas(10, false, true))));
        const turbo::Result<turbo::Result<CopyDetector>> &cref_err = status_error;
        turbo::Result<turbo::Result<CopyDetector>> b_err = cref_err;  // NOLINT
        EXPECT_THAT(b_err, Not(IsOk()));

        turbo::Result<turbo::Result<CopyDetector>> c = std::move(status_or);
        EXPECT_THAT(c, IsOkAndHolds(IsOkAndHolds(CopyDetectorHas(10, true, false))));
        turbo::Result<turbo::Result<CopyDetector>> c_err = std::move(status_error);
        EXPECT_THAT(c_err, Not(IsOk()));
    }

    TEST(Result, NestedStatusOrCopyAndMoveAssignment) {
        turbo::Result<turbo::Result<CopyDetector>> status_or = CopyDetector(10);
        turbo::Result<turbo::Result<CopyDetector>> status_error =
                turbo::invalid_argument_error("foo");
        turbo::Result<turbo::Result<CopyDetector>> a;
        a = status_or;
        EXPECT_THAT(a, IsOkAndHolds(IsOkAndHolds(CopyDetectorHas(10, false, true))));
        a = status_error;
        EXPECT_THAT(a, Not(IsOk()));

        const turbo::Result<turbo::Result<CopyDetector>> &cref = status_or;
        a = cref;
        EXPECT_THAT(a, IsOkAndHolds(IsOkAndHolds(CopyDetectorHas(10, false, true))));
        const turbo::Result<turbo::Result<CopyDetector>> &cref_err = status_error;
        a = cref_err;
        EXPECT_THAT(a, Not(IsOk()));
        a = std::move(status_or);
        EXPECT_THAT(a, IsOkAndHolds(IsOkAndHolds(CopyDetectorHas(10, true, false))));
        a = std::move(status_error);
        EXPECT_THAT(a, Not(IsOk()));
    }

    struct Copyable {
        Copyable() {}

        Copyable(const Copyable &) {}

        Copyable &operator=(const Copyable &) { return *this; }
    };

    struct MoveOnly {
        MoveOnly() {}

        MoveOnly(MoveOnly &&) {}

        MoveOnly &operator=(MoveOnly &&) { return *this; }
    };

    struct NonMovable {
        NonMovable() {}

        NonMovable(const NonMovable &) = delete;

        NonMovable(NonMovable &&) = delete;

        NonMovable &operator=(const NonMovable &) = delete;

        NonMovable &operator=(NonMovable &&) = delete;
    };

    TEST(Result, CopyAndMoveAbility) {
        EXPECT_TRUE(std::is_copy_constructible<Copyable>::value);
        EXPECT_TRUE(std::is_copy_assignable<Copyable>::value);
        EXPECT_TRUE(std::is_move_constructible<Copyable>::value);
        EXPECT_TRUE(std::is_move_assignable<Copyable>::value);
        EXPECT_FALSE(std::is_copy_constructible<MoveOnly>::value);
        EXPECT_FALSE(std::is_copy_assignable<MoveOnly>::value);
        EXPECT_TRUE(std::is_move_constructible<MoveOnly>::value);
        EXPECT_TRUE(std::is_move_assignable<MoveOnly>::value);
        EXPECT_FALSE(std::is_copy_constructible<NonMovable>::value);
        EXPECT_FALSE(std::is_copy_assignable<NonMovable>::value);
        EXPECT_FALSE(std::is_move_constructible<NonMovable>::value);
        EXPECT_FALSE(std::is_move_assignable<NonMovable>::value);
    }

    TEST(Result, StatusOrAnyCopyAndMoveConstructorTests) {
        turbo::Result<turbo::any> status_or = CopyDetector(10);
        turbo::Result<turbo::any> status_error = turbo::invalid_argument_error("foo");
        EXPECT_THAT(
                status_or,
                IsOkAndHolds(AnyWith<CopyDetector>(CopyDetectorHas(10, true, false))));
        turbo::Result<turbo::any> a = status_or;
        EXPECT_THAT(
                a, IsOkAndHolds(AnyWith<CopyDetector>(CopyDetectorHas(10, false, true))));
        turbo::Result<turbo::any> a_err = status_error;
        EXPECT_THAT(a_err, Not(IsOk()));

        const turbo::Result<turbo::any> &cref = status_or;
        // No lint for no-change copy.
        turbo::Result<turbo::any> b = cref;  // NOLINT
        EXPECT_THAT(
                b, IsOkAndHolds(AnyWith<CopyDetector>(CopyDetectorHas(10, false, true))));
        const turbo::Result<turbo::any> &cref_err = status_error;
        // No lint for no-change copy.
        turbo::Result<turbo::any> b_err = cref_err;  // NOLINT
        EXPECT_THAT(b_err, Not(IsOk()));

        turbo::Result<turbo::any> c = std::move(status_or);
        EXPECT_THAT(
                c, IsOkAndHolds(AnyWith<CopyDetector>(CopyDetectorHas(10, true, false))));
        turbo::Result<turbo::any> c_err = std::move(status_error);
        EXPECT_THAT(c_err, Not(IsOk()));
    }

    TEST(Result, StatusOrAnyCopyAndMoveAssignment) {
        turbo::Result<turbo::any> status_or = CopyDetector(10);
        turbo::Result<turbo::any> status_error = turbo::invalid_argument_error("foo");
        turbo::Result<turbo::any> a;
        a = status_or;
        EXPECT_THAT(
                a, IsOkAndHolds(AnyWith<CopyDetector>(CopyDetectorHas(10, false, true))));
        a = status_error;
        EXPECT_THAT(a, Not(IsOk()));

        const turbo::Result<turbo::any> &cref = status_or;
        a = cref;
        EXPECT_THAT(
                a, IsOkAndHolds(AnyWith<CopyDetector>(CopyDetectorHas(10, false, true))));
        const turbo::Result<turbo::any> &cref_err = status_error;
        a = cref_err;
        EXPECT_THAT(a, Not(IsOk()));
        a = std::move(status_or);
        EXPECT_THAT(
                a, IsOkAndHolds(AnyWith<CopyDetector>(CopyDetectorHas(10, true, false))));
        a = std::move(status_error);
        EXPECT_THAT(a, Not(IsOk()));
    }

    TEST(Result, StatusOrCopyAndMoveTestsConstructor) {
        turbo::Result<CopyDetector> status_or(10);
        ASSERT_THAT(status_or, IsOkAndHolds(CopyDetectorHas(10, false, false)));
        turbo::Result<CopyDetector> a(status_or);
        EXPECT_THAT(a, IsOkAndHolds(CopyDetectorHas(10, false, true)));
        const turbo::Result<CopyDetector> &cref = status_or;
        turbo::Result<CopyDetector> b(cref);  // NOLINT
        EXPECT_THAT(b, IsOkAndHolds(CopyDetectorHas(10, false, true)));
        turbo::Result<CopyDetector> c(std::move(status_or));
        EXPECT_THAT(c, IsOkAndHolds(CopyDetectorHas(10, true, false)));
    }

    TEST(Result, StatusOrCopyAndMoveTestsAssignment) {
        turbo::Result<CopyDetector> status_or(10);
        ASSERT_THAT(status_or, IsOkAndHolds(CopyDetectorHas(10, false, false)));
        turbo::Result<CopyDetector> a;
        a = status_or;
        EXPECT_THAT(a, IsOkAndHolds(CopyDetectorHas(10, false, true)));
        const turbo::Result<CopyDetector> &cref = status_or;
        turbo::Result<CopyDetector> b;
        b = cref;
        EXPECT_THAT(b, IsOkAndHolds(CopyDetectorHas(10, false, true)));
        turbo::Result<CopyDetector> c;
        c = std::move(status_or);
        EXPECT_THAT(c, IsOkAndHolds(CopyDetectorHas(10, true, false)));
    }

    TEST(Result, TurboAnyAssignment) {
        EXPECT_FALSE((std::is_assignable<turbo::Result<turbo::any>,
                turbo::Result<int>>::value));
        turbo::Result<turbo::any> status_or;
        status_or = turbo::invalid_argument_error("foo");
        EXPECT_THAT(status_or, Not(IsOk()));
    }

    TEST(Result, ImplicitAssignment) {
        turbo::Result<turbo::variant<int, std::string>> status_or;
        status_or = 10;
        EXPECT_THAT(status_or, IsOkAndHolds(VariantWith<int>(10)));
    }

    TEST(Result, SelfDirectInitAssignment) {
        turbo::Result<std::vector<int>> status_or = {{10, 20, 30}};
        status_or = *status_or;
        EXPECT_THAT(status_or, IsOkAndHolds(ElementsAre(10, 20, 30)));
    }

    TEST(Result, ImplicitCastFromInitializerList) {
        turbo::Result<std::vector<int>> status_or = {{10, 20, 30}};
        EXPECT_THAT(status_or, IsOkAndHolds(ElementsAre(10, 20, 30)));
    }

    TEST(Result, UniquePtrImplicitAssignment) {
        turbo::Result<std::unique_ptr<Base1>> status_or;
        status_or = turbo::make_unique<Derived>();
        EXPECT_THAT(status_or, IsOkAndHolds(Ne(nullptr)));
    }

    TEST(Result, Pointer) {
        struct A {
        };
        struct B : public A {
        };
        struct C : private A {
        };

        EXPECT_TRUE((std::is_constructible<turbo::Result<A *>, B *>::value));
        EXPECT_TRUE((std::is_convertible<B *, turbo::Result<A *>>::value));
        EXPECT_FALSE((std::is_constructible<turbo::Result<A *>, C *>::value));
        EXPECT_FALSE((std::is_convertible<C *, turbo::Result<A *>>::value));
    }

    TEST(Result, TestAssignmentStatusNotOkConverting) {
        // Copy assignment
        {
            const turbo::Status expected = turbo::cancelled_error();
            turbo::Result<int> source(expected);

            turbo::Result<double> target;
            target = source;

            EXPECT_FALSE(target.ok());
            EXPECT_EQ(expected, target.status());

            EXPECT_FALSE(source.ok());
            EXPECT_EQ(expected, source.status());
        }

        // Move assignment
        {
            const turbo::Status expected = turbo::cancelled_error();
            turbo::Result<int> source(expected);

            turbo::Result<double> target;
            target = std::move(source);

            EXPECT_FALSE(target.ok());
            EXPECT_EQ(expected, target.status());

            EXPECT_FALSE(source.ok());
            EXPECT_EQ(source.status().code(), turbo::StatusCode::kInternal);
        }
    }

    TEST(Result, SelfAssignment) {
        // Copy-assignment, status OK
        {
            // A string long enough that it's likely to defeat any inline representation
            // optimization.
            const std::string long_str(128, 'a');

            turbo::Result<std::string> so = long_str;
            so = *&so;

            ASSERT_TRUE(so.ok());
            EXPECT_THAT(so.status(), IsOk());
            EXPECT_EQ(long_str, *so);
        }

        // Copy-assignment, error status
        {
            turbo::Result<int> so = turbo::not_found_error("taco");
            so = *&so;

            EXPECT_FALSE(so.ok());
            EXPECT_EQ(so.status().code(), turbo::StatusCode::kNotFound);
            EXPECT_EQ(so.status().message(), "taco");
        }

        // Move-assignment with copyable type, status OK
        {
            turbo::Result<int> so = 17;

            // Fool the compiler, which otherwise complains.
            auto &same = so;
            so = std::move(same);

            ASSERT_TRUE(so.ok());
            EXPECT_THAT(so.status(), IsOk());
            EXPECT_EQ(17, *so);
        }

        // Move-assignment with copyable type, error status
        {
            turbo::Result<int> so = turbo::not_found_error("taco");

            // Fool the compiler, which otherwise complains.
            auto &same = so;
            so = std::move(same);

            EXPECT_FALSE(so.ok());
            EXPECT_EQ(so.status().code(), turbo::StatusCode::kNotFound);
            EXPECT_EQ(so.status().message(), "taco");
        }

        // Move-assignment with non-copyable type, status OK
        {
            const auto raw = new int(17);
            turbo::Result<std::unique_ptr<int>> so = turbo::WrapUnique(raw);

            // Fool the compiler, which otherwise complains.
            auto &same = so;
            so = std::move(same);

            ASSERT_TRUE(so.ok());
            EXPECT_THAT(so.status(), IsOk());
            EXPECT_EQ(raw, so->get());
        }

        // Move-assignment with non-copyable type, error status
        {
            turbo::Result<std::unique_ptr<int>> so = turbo::not_found_error("taco");

            // Fool the compiler, which otherwise complains.
            auto &same = so;
            so = std::move(same);

            EXPECT_FALSE(so.ok());
            EXPECT_EQ(so.status().code(), turbo::StatusCode::kNotFound);
            EXPECT_EQ(so.status().message(), "taco");
        }
    }

// These types form the overload sets of the constructors and the assignment
// operators of `MockValue`. They distinguish construction from assignment,
// lvalue from rvalue.
    struct FromConstructibleAssignableLvalue {
    };
    struct FromConstructibleAssignableRvalue {
    };
    struct FromImplicitConstructibleOnly {
    };
    struct FromAssignableOnly {
    };

// This class is for testing the forwarding value assignments of `Result`.
// `from_rvalue` indicates whether the constructor or the assignment taking
// rvalue reference is called. `from_assignment` indicates whether any
// assignment is called.
    struct MockValue {
        // Constructs `MockValue` from `FromConstructibleAssignableLvalue`.
        MockValue(const FromConstructibleAssignableLvalue &)  // NOLINT
                : from_rvalue(false), assigned(false) {}

        // Constructs `MockValue` from `FromConstructibleAssignableRvalue`.
        MockValue(FromConstructibleAssignableRvalue &&)  // NOLINT
                : from_rvalue(true), assigned(false) {}

        // Constructs `MockValue` from `FromImplicitConstructibleOnly`.
        // `MockValue` is not assignable from `FromImplicitConstructibleOnly`.
        MockValue(const FromImplicitConstructibleOnly &)  // NOLINT
                : from_rvalue(false), assigned(false) {}

        // Assigns `FromConstructibleAssignableLvalue`.
        MockValue &operator=(const FromConstructibleAssignableLvalue &) {
            from_rvalue = false;
            assigned = true;
            return *this;
        }

        // Assigns `FromConstructibleAssignableRvalue` (rvalue only).
        MockValue &operator=(FromConstructibleAssignableRvalue &&) {
            from_rvalue = true;
            assigned = true;
            return *this;
        }

        // Assigns `FromAssignableOnly`, but not constructible from
        // `FromAssignableOnly`.
        MockValue &operator=(const FromAssignableOnly &) {
            from_rvalue = false;
            assigned = true;
            return *this;
        }

        bool from_rvalue;
        bool assigned;
    };

// operator=(U&&)
    TEST(Result, PerfectForwardingAssignment) {
        // U == T
        constexpr int kValue1 = 10, kValue2 = 20;
        turbo::Result<CopyDetector> status_or;
        CopyDetector lvalue(kValue1);
        status_or = lvalue;
        EXPECT_THAT(status_or, IsOkAndHolds(CopyDetectorHas(kValue1, false, true)));
        status_or = CopyDetector(kValue2);
        EXPECT_THAT(status_or, IsOkAndHolds(CopyDetectorHas(kValue2, true, false)));

        // U != T
        EXPECT_TRUE(
                (std::is_assignable<turbo::Result<MockValue> &,
                        const FromConstructibleAssignableLvalue &>::value));
        EXPECT_TRUE((std::is_assignable<turbo::Result<MockValue> &,
                FromConstructibleAssignableLvalue &&>::value));
        EXPECT_FALSE(
                (std::is_assignable<turbo::Result<MockValue> &,
                        const FromConstructibleAssignableRvalue &>::value));
        EXPECT_TRUE((std::is_assignable<turbo::Result<MockValue> &,
                FromConstructibleAssignableRvalue &&>::value));
        EXPECT_TRUE(
                (std::is_assignable<turbo::Result<MockValue> &,
                        const FromImplicitConstructibleOnly &>::value));
        EXPECT_FALSE((std::is_assignable<turbo::Result<MockValue> &,
                const FromAssignableOnly &>::value));

        turbo::Result<MockValue> from_lvalue(FromConstructibleAssignableLvalue{});
        EXPECT_FALSE(from_lvalue->from_rvalue);
        EXPECT_FALSE(from_lvalue->assigned);
        from_lvalue = FromConstructibleAssignableLvalue{};
        EXPECT_FALSE(from_lvalue->from_rvalue);
        EXPECT_TRUE(from_lvalue->assigned);

        turbo::Result<MockValue> from_rvalue(FromConstructibleAssignableRvalue{});
        EXPECT_TRUE(from_rvalue->from_rvalue);
        EXPECT_FALSE(from_rvalue->assigned);
        from_rvalue = FromConstructibleAssignableRvalue{};
        EXPECT_TRUE(from_rvalue->from_rvalue);
        EXPECT_TRUE(from_rvalue->assigned);

        turbo::Result<MockValue> from_implicit_constructible(
                FromImplicitConstructibleOnly{});
        EXPECT_FALSE(from_implicit_constructible->from_rvalue);
        EXPECT_FALSE(from_implicit_constructible->assigned);
        // construct a temporary `Result` object and invoke the `Result` move
        // assignment operator.
        from_implicit_constructible = FromImplicitConstructibleOnly{};
        EXPECT_FALSE(from_implicit_constructible->from_rvalue);
        EXPECT_FALSE(from_implicit_constructible->assigned);
    }

    TEST(Result, TestStatus) {
        turbo::Result<int> good(4);
        EXPECT_TRUE(good.ok());
        turbo::Result<int> bad(turbo::cancelled_error());
        EXPECT_FALSE(bad.ok());
        EXPECT_EQ(bad.status().code(), turbo::StatusCode::kCancelled);
    }

    TEST(Result, OperatorStarRefQualifiers) {
        static_assert(
                std::is_same<const int &,
                        decltype(*std::declval<const turbo::Result<int> &>())>(),
                "Unexpected ref-qualifiers");
        static_assert(
                std::is_same<int &, decltype(*std::declval<turbo::Result<int> &>())>(),
                "Unexpected ref-qualifiers");
        static_assert(
                std::is_same<const int &&,
                        decltype(*std::declval<const turbo::Result<int> &&>())>(),
                "Unexpected ref-qualifiers");
        static_assert(
                std::is_same<int &&, decltype(*std::declval<turbo::Result<int> &&>())>(),
                "Unexpected ref-qualifiers");
    }

    TEST(Result, OperatorStar) {
        const turbo::Result<std::string> const_lvalue("hello");
        EXPECT_EQ("hello", *const_lvalue);

        turbo::Result<std::string> lvalue("hello");
        EXPECT_EQ("hello", *lvalue);

        // Note: Recall that std::move() is equivalent to a static_cast to an rvalue
        // reference type.
        const turbo::Result<std::string> const_rvalue("hello");
        EXPECT_EQ("hello", *std::move(const_rvalue));  // NOLINT

        turbo::Result<std::string> rvalue("hello");
        EXPECT_EQ("hello", *std::move(rvalue));
    }

    TEST(Result, OperatorArrowQualifiers) {
        static_assert(
                std::is_same<
                        const int *,
                        decltype(std::declval<const turbo::Result<int> &>().operator->())>(),
                "Unexpected qualifiers");
        static_assert(
                std::is_same<
                        int *, decltype(std::declval<turbo::Result<int> &>().operator->())>(),
                "Unexpected qualifiers");
        static_assert(
                std::is_same<
                        const int *,
                        decltype(std::declval<const turbo::Result<int> &&>().operator->())>(),
                "Unexpected qualifiers");
        static_assert(
                std::is_same<
                        int *, decltype(std::declval<turbo::Result<int> &&>().operator->())>(),
                "Unexpected qualifiers");
    }

    TEST(Result, OperatorArrow) {
        const turbo::Result<std::string> const_lvalue("hello");
        EXPECT_EQ(std::string("hello"), const_lvalue->c_str());

        turbo::Result<std::string> lvalue("hello");
        EXPECT_EQ(std::string("hello"), lvalue->c_str());
    }

    TEST(Result, RValueStatus) {
        turbo::Result<int> so(turbo::not_found_error("taco"));
        const turbo::Status s = std::move(so).status();

        EXPECT_EQ(s.code(), turbo::StatusCode::kNotFound);
        EXPECT_EQ(s.message(), "taco");

        // Check that !ok() still implies !status().ok(), even after moving out of the
        // object. See the note on the rvalue ref-qualified status method.
        EXPECT_FALSE(so.ok());  // NOLINT
        EXPECT_FALSE(so.status().ok());
        EXPECT_EQ(so.status().code(), turbo::StatusCode::kInternal);
        EXPECT_EQ(so.status().message(), "Status accessed after move.");
    }

    TEST(Result, TestValue) {
        const int kI = 4;
        turbo::Result<int> thing(kI);
        EXPECT_EQ(kI, *thing);
    }

    TEST(Result, TestValueConst) {
        const int kI = 4;
        const turbo::Result<int> thing(kI);
        EXPECT_EQ(kI, *thing);
    }

    TEST(Result, TestPointerDefaultCtor) {
        turbo::Result<int *> thing;
        EXPECT_FALSE(thing.ok());
        EXPECT_EQ(thing.status().code(), turbo::StatusCode::kUnknown);
    }

    TEST(Result, TestPointerStatusCtor) {
        turbo::Result<int *> thing(turbo::cancelled_error());
        EXPECT_FALSE(thing.ok());
        EXPECT_EQ(thing.status().code(), turbo::StatusCode::kCancelled);
    }

    TEST(Result, TestPointerValueCtor) {
        const int kI = 4;

        // Construction from a non-null pointer
        {
            turbo::Result<const int *> so(&kI);
            EXPECT_TRUE(so.ok());
            EXPECT_THAT(so.status(), IsOk());
            EXPECT_EQ(&kI, *so);
        }

        // Construction from a null pointer constant
        {
            turbo::Result<const int *> so(nullptr);
            EXPECT_TRUE(so.ok());
            EXPECT_THAT(so.status(), IsOk());
            EXPECT_EQ(nullptr, *so);
        }

        // Construction from a non-literal null pointer
        {
            const int *const p = nullptr;

            turbo::Result<const int *> so(p);
            EXPECT_TRUE(so.ok());
            EXPECT_THAT(so.status(), IsOk());
            EXPECT_EQ(nullptr, *so);
        }
    }

    TEST(Result, TestPointerCopyCtorStatusOk) {
        const int kI = 0;
        turbo::Result<const int *> original(&kI);
        turbo::Result<const int *> copy(original);
        EXPECT_THAT(copy.status(), IsOk());
        EXPECT_EQ(*original, *copy);
    }

    TEST(Result, TestPointerCopyCtorStatusNotOk) {
        turbo::Result<int *> original(turbo::cancelled_error());
        turbo::Result<int *> copy(original);
        EXPECT_EQ(copy.status().code(), turbo::StatusCode::kCancelled);
    }

    TEST(Result, TestPointerCopyCtorStatusOKConverting) {
        Derived derived;
        turbo::Result<Derived *> original(&derived);
        turbo::Result<Base2 *> copy(original);
        EXPECT_THAT(copy.status(), IsOk());
        EXPECT_EQ(static_cast<const Base2 *>(*original), *copy);
    }

    TEST(Result, TestPointerCopyCtorStatusNotOkConverting) {
        turbo::Result<Derived *> original(turbo::cancelled_error());
        turbo::Result<Base2 *> copy(original);
        EXPECT_EQ(copy.status().code(), turbo::StatusCode::kCancelled);
    }

    TEST(Result, TestPointerAssignmentStatusOk) {
        const int kI = 0;
        turbo::Result<const int *> source(&kI);
        turbo::Result<const int *> target;
        target = source;
        EXPECT_THAT(target.status(), IsOk());
        EXPECT_EQ(*source, *target);
    }

    TEST(Result, TestPointerAssignmentStatusNotOk) {
        turbo::Result<int *> source(turbo::cancelled_error());
        turbo::Result<int *> target;
        target = source;
        EXPECT_EQ(target.status().code(), turbo::StatusCode::kCancelled);
    }

    TEST(Result, TestPointerAssignmentStatusOKConverting) {
        Derived derived;
        turbo::Result<Derived *> source(&derived);
        turbo::Result<Base2 *> target;
        target = source;
        EXPECT_THAT(target.status(), IsOk());
        EXPECT_EQ(static_cast<const Base2 *>(*source), *target);
    }

    TEST(Result, TestPointerAssignmentStatusNotOkConverting) {
        turbo::Result<Derived *> source(turbo::cancelled_error());
        turbo::Result<Base2 *> target;
        target = source;
        EXPECT_EQ(target.status(), source.status());
    }

    TEST(Result, TestPointerStatus) {
        const int kI = 0;
        turbo::Result<const int *> good(&kI);
        EXPECT_TRUE(good.ok());
        turbo::Result<const int *> bad(turbo::cancelled_error());
        EXPECT_EQ(bad.status().code(), turbo::StatusCode::kCancelled);
    }

    TEST(Result, TestPointerValue) {
        const int kI = 0;
        turbo::Result<const int *> thing(&kI);
        EXPECT_EQ(&kI, *thing);
    }

    TEST(Result, TestPointerValueConst) {
        const int kI = 0;
        const turbo::Result<const int *> thing(&kI);
        EXPECT_EQ(&kI, *thing);
    }

    TEST(Result, StatusOrVectorOfUniquePointerCanReserveAndResize) {
        using EvilType = std::vector<std::unique_ptr<int>>;
        static_assert(std::is_copy_constructible<EvilType>::value, "");
        std::vector<::turbo::Result<EvilType>> v(5);
        v.reserve(v.capacity() + 10);
        v.resize(v.capacity() + 10);
    }

    TEST(Result, ConstPayload) {
        // A reduced version of a problematic type found in the wild. All of the
        // operations below should compile.
        turbo::Result<const int> a;

        // Copy-construction
        turbo::Result<const int> b(a);

        // Copy-assignment
        EXPECT_FALSE(std::is_copy_assignable<turbo::Result<const int>>::value);

        // Move-construction
        turbo::Result<const int> c(std::move(a));

        // Move-assignment
        EXPECT_FALSE(std::is_move_assignable<turbo::Result<const int>>::value);
    }

    TEST(Result, MapToStatusOrUniquePtr) {
        // A reduced version of a problematic type found in the wild. All of the
        // operations below should compile.
        using MapType = std::map<std::string, turbo::Result<std::unique_ptr<int>>>;

        MapType a;

        // Move-construction
        MapType b(std::move(a));

        // Move-assignment
        a = std::move(b);
    }

    TEST(Result, ValueOrOk) {
        const turbo::Result<int> status_or = 0;
        EXPECT_EQ(status_or.value_or(-1), 0);
    }

    TEST(Result, ValueOrDefault) {
        const turbo::Result<int> status_or = turbo::cancelled_error();
        EXPECT_EQ(status_or.value_or(-1), -1);
    }

    TEST(Result, MoveOnlyValueOrOk) {
        EXPECT_THAT(turbo::Result<std::unique_ptr<int>>(turbo::make_unique<int>(0))
                            .value_or(turbo::make_unique<int>(-1)),
                    Pointee(0));
    }

    TEST(Result, MoveOnlyValueOrDefault) {
        EXPECT_THAT(turbo::Result<std::unique_ptr<int>>(turbo::cancelled_error())
                            .value_or(turbo::make_unique<int>(-1)),
                    Pointee(-1));
    }

    static turbo::Result<int> MakeStatus() { return 100; }

    TEST(Result, TestIgnoreError) { MakeStatus().ignore_error(); }

    TEST(Result, EqualityOperator) {
        constexpr size_t kNumCases = 4;
        std::array<turbo::Result<int>, kNumCases> group1 = {
                turbo::Result<int>(1), turbo::Result<int>(2),
                turbo::Result<int>(turbo::invalid_argument_error("msg")),
                turbo::Result<int>(turbo::internal_error("msg"))};
        std::array<turbo::Result<int>, kNumCases> group2 = {
                turbo::Result<int>(1), turbo::Result<int>(2),
                turbo::Result<int>(turbo::invalid_argument_error("msg")),
                turbo::Result<int>(turbo::internal_error("msg"))};
        for (size_t i = 0; i < kNumCases; ++i) {
            for (size_t j = 0; j < kNumCases; ++j) {
                if (i == j) {
                    EXPECT_TRUE(group1[i] == group2[j]);
                    EXPECT_FALSE(group1[i] != group2[j]);
                } else {
                    EXPECT_FALSE(group1[i] == group2[j]);
                    EXPECT_TRUE(group1[i] != group2[j]);
                }
            }
        }
    }

    struct MyType {
        bool operator==(const MyType &) const { return true; }
    };

    enum class ConvTraits {
        kNone = 0, kImplicit = 1, kExplicit = 2
    };

// This class has conversion operator to `Result<T>` based on value of
// `conv_traits`.
    template<typename T, ConvTraits conv_traits = ConvTraits::kNone>
    struct StatusOrConversionBase {
    };

    template<typename T>
    struct StatusOrConversionBase<T, ConvTraits::kImplicit> {
        operator turbo::Result<T>() const & {  // NOLINT
            return turbo::invalid_argument_error("conversion to turbo::Result");
        }

        operator turbo::Result<T>() && {  // NOLINT
            return turbo::invalid_argument_error("conversion to turbo::Result");
        }
    };

    template<typename T>
    struct StatusOrConversionBase<T, ConvTraits::kExplicit> {
        explicit operator turbo::Result<T>() const & {
            return turbo::invalid_argument_error("conversion to turbo::Result");
        }

        explicit operator turbo::Result<T>() && {
            return turbo::invalid_argument_error("conversion to turbo::Result");
        }
    };

// This class has conversion operator to `T` based on the value of
// `conv_traits`.
    template<typename T, ConvTraits conv_traits = ConvTraits::kNone>
    struct ConversionBase {
    };

    template<typename T>
    struct ConversionBase<T, ConvTraits::kImplicit> {
        operator T() const & { return t; }         // NOLINT
        operator T() && { return std::move(t); }  // NOLINT
        T t;
    };

    template<typename T>
    struct ConversionBase<T, ConvTraits::kExplicit> {
        explicit operator T() const & { return t; }

        explicit operator T() && { return std::move(t); }

        T t;
    };

// This class has conversion operator to `turbo::Status` based on the value of
// `conv_traits`.
    template<ConvTraits conv_traits = ConvTraits::kNone>
    struct StatusConversionBase {
    };

    template<>
    struct StatusConversionBase<ConvTraits::kImplicit> {
        operator turbo::Status() const & {  // NOLINT
            return turbo::internal_error("conversion to Status");
        }

        operator turbo::Status() && {  // NOLINT
            return turbo::internal_error("conversion to Status");
        }
    };

    template<>
    struct StatusConversionBase<ConvTraits::kExplicit> {
        explicit operator turbo::Status() const & {  // NOLINT
            return turbo::internal_error("conversion to Status");
        }

        explicit operator turbo::Status() && {  // NOLINT
            return turbo::internal_error("conversion to Status");
        }
    };

    static constexpr int kConvToStatus = 1;
    static constexpr int kConvToStatusOr = 2;
    static constexpr int kConvToT = 4;
    static constexpr int kConvExplicit = 8;

    constexpr ConvTraits GetConvTraits(int bit, int config) {
        return (config & bit) == 0
               ? ConvTraits::kNone
               : ((config & kConvExplicit) == 0 ? ConvTraits::kImplicit
                                                : ConvTraits::kExplicit);
    }

// This class conditionally has conversion operator to `turbo::Status`, `T`,
// `Result<T>`, based on values of the template parameters.
    template<typename T, int config>
    struct CustomType
            : StatusOrConversionBase<T, GetConvTraits(kConvToStatusOr, config)>,
              ConversionBase<T, GetConvTraits(kConvToT, config)>,
              StatusConversionBase<GetConvTraits(kConvToStatus, config)> {
    };

    struct ConvertibleToAnyStatusOr {
        template<typename T>
        operator turbo::Result<T>() const {  // NOLINT
            return turbo::invalid_argument_error("Conversion to turbo::Result");
        }
    };

// Test the rank of overload resolution for `Result<T>` constructor and
// assignment, from highest to lowest:
// 1. T/Status
// 2. U that has conversion operator to turbo::Result<T>
// 3. U that is convertible to Status
// 4. U that is convertible to T
    TEST(Result, ConstructionFromT) {
        // Construct turbo::Result<T> from T when T is convertible to
        // turbo::Result<T>
        {
            ConvertibleToAnyStatusOr v;
            turbo::Result<ConvertibleToAnyStatusOr> statusor(v);
            EXPECT_TRUE(statusor.ok());
        }
        {
            ConvertibleToAnyStatusOr v;
            turbo::Result<ConvertibleToAnyStatusOr> statusor = v;
            EXPECT_TRUE(statusor.ok());
        }
        // Construct turbo::Result<T> from T when T is explicitly convertible to
        // Status
        {
            CustomType<MyType, kConvToStatus | kConvExplicit> v;
            turbo::Result<CustomType<MyType, kConvToStatus | kConvExplicit>> statusor(
                    v);
            EXPECT_TRUE(statusor.ok());
        }
        {
            CustomType<MyType, kConvToStatus | kConvExplicit> v;
            turbo::Result<CustomType<MyType, kConvToStatus | kConvExplicit>> statusor =
                    v;
            EXPECT_TRUE(statusor.ok());
        }
    }

// Construct turbo::Result<T> from U when U is explicitly convertible to T
    TEST(Result, ConstructionFromTypeConvertibleToT) {
        {
            CustomType<MyType, kConvToT | kConvExplicit> v;
            turbo::Result<MyType> statusor(v);
            EXPECT_TRUE(statusor.ok());
        }
        {
            CustomType<MyType, kConvToT> v;
            turbo::Result<MyType> statusor = v;
            EXPECT_TRUE(statusor.ok());
        }
    }

// Construct turbo::Result<T> from U when U has explicit conversion operator to
// turbo::Result<T>
    TEST(Result, ConstructionFromTypeWithConversionOperatorToStatusOrT) {
        {
            CustomType<MyType, kConvToStatusOr | kConvExplicit> v;
            turbo::Result<MyType> statusor(v);
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToT | kConvToStatusOr | kConvExplicit> v;
            turbo::Result<MyType> statusor(v);
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToStatusOr | kConvToStatus | kConvExplicit> v;
            turbo::Result<MyType> statusor(v);
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType,
                    kConvToT | kConvToStatusOr | kConvToStatus | kConvExplicit>
                    v;
            turbo::Result<MyType> statusor(v);
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToStatusOr> v;
            turbo::Result<MyType> statusor = v;
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToT | kConvToStatusOr> v;
            turbo::Result<MyType> statusor = v;
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToStatusOr | kConvToStatus> v;
            turbo::Result<MyType> statusor = v;
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToT | kConvToStatusOr | kConvToStatus> v;
            turbo::Result<MyType> statusor = v;
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
    }

    TEST(Result, ConstructionFromTypeConvertibleToStatus) {
        // Construction fails because conversion to `Status` is explicit.
        {
            CustomType<MyType, kConvToStatus | kConvExplicit> v;
            turbo::Result<MyType> statusor(v);
            EXPECT_FALSE(statusor.ok());
            EXPECT_EQ(statusor.status(), static_cast<turbo::Status>(v));
        }
        {
            CustomType<MyType, kConvToT | kConvToStatus | kConvExplicit> v;
            turbo::Result<MyType> statusor(v);
            EXPECT_FALSE(statusor.ok());
            EXPECT_EQ(statusor.status(), static_cast<turbo::Status>(v));
        }
        {
            CustomType<MyType, kConvToStatus> v;
            turbo::Result<MyType> statusor = v;
            EXPECT_FALSE(statusor.ok());
            EXPECT_EQ(statusor.status(), static_cast<turbo::Status>(v));
        }
        {
            CustomType<MyType, kConvToT | kConvToStatus> v;
            turbo::Result<MyType> statusor = v;
            EXPECT_FALSE(statusor.ok());
            EXPECT_EQ(statusor.status(), static_cast<turbo::Status>(v));
        }
    }

    TEST(Result, AssignmentFromT) {
        // Assign to turbo::Result<T> from T when T is convertible to
        // turbo::Result<T>
        {
            ConvertibleToAnyStatusOr v;
            turbo::Result<ConvertibleToAnyStatusOr> statusor;
            statusor = v;
            EXPECT_TRUE(statusor.ok());
        }
        // Assign to turbo::Result<T> from T when T is convertible to Status
        {
            CustomType<MyType, kConvToStatus> v;
            turbo::Result<CustomType<MyType, kConvToStatus>> statusor;
            statusor = v;
            EXPECT_TRUE(statusor.ok());
        }
    }

    TEST(Result, AssignmentFromTypeConvertibleToT) {
        // Assign to turbo::Result<T> from U when U is convertible to T
        {
            CustomType<MyType, kConvToT> v;
            turbo::Result<MyType> statusor;
            statusor = v;
            EXPECT_TRUE(statusor.ok());
        }
    }

    TEST(Result, AssignmentFromTypeWithConversionOperatortoStatusOrT) {
        // Assign to turbo::Result<T> from U when U has conversion operator to
        // turbo::Result<T>
        {
            CustomType<MyType, kConvToStatusOr> v;
            turbo::Result<MyType> statusor;
            statusor = v;
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToT | kConvToStatusOr> v;
            turbo::Result<MyType> statusor;
            statusor = v;
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToStatusOr | kConvToStatus> v;
            turbo::Result<MyType> statusor;
            statusor = v;
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
        {
            CustomType<MyType, kConvToT | kConvToStatusOr | kConvToStatus> v;
            turbo::Result<MyType> statusor;
            statusor = v;
            EXPECT_EQ(statusor, v.operator turbo::Result<MyType>());
        }
    }

    TEST(Result, AssignmentFromTypeConvertibleToStatus) {
        // Assign to turbo::Result<T> from U when U is convertible to Status
        {
            CustomType<MyType, kConvToStatus> v;
            turbo::Result<MyType> statusor;
            statusor = v;
            EXPECT_FALSE(statusor.ok());
            EXPECT_EQ(statusor.status(), static_cast<turbo::Status>(v));
        }
        {
            CustomType<MyType, kConvToT | kConvToStatus> v;
            turbo::Result<MyType> statusor;
            statusor = v;
            EXPECT_FALSE(statusor.ok());
            EXPECT_EQ(statusor.status(), static_cast<turbo::Status>(v));
        }
    }

    TEST(Result, StatusAssignmentFromStatusError) {
        turbo::Result<turbo::Status> statusor;
        statusor.AssignStatus(turbo::cancelled_error());

        EXPECT_FALSE(statusor.ok());
        EXPECT_EQ(statusor.status(), turbo::cancelled_error());
    }

#if GTEST_HAS_DEATH_TEST
    TEST(Result, StatusAssignmentFromStatusOk) {
        EXPECT_DEBUG_DEATH(
                {
                    turbo::Result<turbo::Status> statusor;
                    // This will DCHECK.
                    statusor.AssignStatus(turbo::OkStatus());
                    // In optimized mode, we are actually going to get error::INTERNAL for
                    // status here, rather than crashing, so check that.
                    EXPECT_FALSE(statusor.ok());
                    EXPECT_EQ(statusor.status().code(), turbo::StatusCode::kInternal);
                },
                "An OK status is not a valid constructor argument to Result<T>");
    }

#endif

    TEST(Result, StatusAssignmentFromTypeConvertibleToStatus) {
        CustomType<MyType, kConvToStatus> v;
        turbo::Result<MyType> statusor;
        statusor.AssignStatus(v);

        EXPECT_FALSE(statusor.ok());
        EXPECT_EQ(statusor.status(), static_cast<turbo::Status>(v));
    }

    struct PrintTestStruct {
        friend std::ostream &operator<<(std::ostream &os, const PrintTestStruct &) {
            return os << "ostream";
        }

        template<typename Sink>
        friend void turbo_stringify(Sink &sink, const PrintTestStruct &) {
            sink.Append("stringify");
        }
    };

    TEST(Result, OkPrinting) {
        turbo::Result<PrintTestStruct> print_me = PrintTestStruct{};
        std::stringstream stream;
        stream << print_me;
        EXPECT_EQ(stream.str(), "ostream");
        EXPECT_EQ(turbo::str_cat(print_me), "stringify");
    }

    TEST(Result, ErrorPrinting) {
        turbo::Result<PrintTestStruct> print_me = turbo::unknown_error("error");
        std::stringstream stream;
        stream << print_me;
        const auto error_matcher =
                AllOf(HasSubstr("UNKNOWN"), HasSubstr("error"),
                      AnyOf(AllOf(StartsWith("("), EndsWith(")")),
                            AllOf(StartsWith("["), EndsWith("]"))));
        EXPECT_THAT(stream.str(), error_matcher);
        EXPECT_THAT(turbo::str_cat(print_me), error_matcher);
    }

}  // namespace
