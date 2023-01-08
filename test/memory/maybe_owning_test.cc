
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/memory/maybe_owning.h"
#include "testing/gtest_wrap.h"

using namespace std::literals;

namespace flare {

    int dtor_called = 0;

    struct C {
        ~C() { ++dtor_called; }
    };

    struct Base {
        virtual ~Base() = default;
    };

    struct Derived : Base {
    };

    void AcceptMaybeOwningArgument(maybe_owning_argument<int> ptr) {
        // NOTHING.
    }

    void AcceptMaybeOwningArgumentBase(maybe_owning_argument<Base> ptr) {}

    TEST(maybe_owning, Owning) {
        dtor_called = 0;
        C *ptr = new C();
        {
            maybe_owning<C> ppp(ptr, true);
            ASSERT_EQ(0, dtor_called);
        }
        ASSERT_EQ(1, dtor_called);
    }

    TEST(maybe_owning, Owning2) {
        dtor_called = 0;
        C *ptr = new C();
        {
            maybe_owning<C> ppp(owning, ptr);
            ASSERT_EQ(0, dtor_called);
        }
        ASSERT_EQ(1, dtor_called);
    }

    TEST(maybe_owning, NonOwning) {
        dtor_called = 0;
        C *ptr = new C();
        {
            maybe_owning<C> ppp(ptr, false);
            ASSERT_EQ(0, dtor_called);
        }
        ASSERT_EQ(0, dtor_called);
        delete ptr;
        ASSERT_EQ(1, dtor_called);
    }

    TEST(maybe_owning, NonOwning2) {
        dtor_called = 0;
        C *ptr = new C();
        {
            maybe_owning<C> ppp(non_owning, ptr);
            ASSERT_EQ(0, dtor_called);
        }
        ASSERT_EQ(0, dtor_called);
        delete ptr;
        ASSERT_EQ(1, dtor_called);
    }

    TEST(maybe_owning, FromUniquePtr) {
        dtor_called = 0;
        auto ptr = std::make_unique<C>();
        {
            maybe_owning<C> ppp(std::move(ptr));
            ASSERT_EQ(0, dtor_called);
        }
        ASSERT_EQ(1, dtor_called);
    }

    // This UT shouldn't crash.
    TEST(maybe_owning, FromEmptyUniquePtr) {
        dtor_called = 0;
        std::unique_ptr<C> p;
        {
            maybe_owning<C> ppp(std::move(p));
            ASSERT_EQ(0, dtor_called);
        }
        ASSERT_EQ(0, dtor_called);
    }

    TEST(maybe_owning, Move) {
        dtor_called = 0;
        {
            maybe_owning<C> ppp(new C(), true);
            ASSERT_EQ(0, dtor_called);
            maybe_owning<C> ppp2(std::move(ppp));
            ASSERT_FALSE(!!ppp);
            ASSERT_TRUE(!!ppp2);
            ASSERT_EQ(0, dtor_called);
            maybe_owning<C> ppp3;
            ASSERT_FALSE(!!ppp3);
            ppp3 = std::move(ppp);
            ASSERT_FALSE(!!ppp3);
            ppp3 = std::move(ppp2);
            ASSERT_FALSE(!!ppp2);
            ASSERT_TRUE(!!ppp3);
            ASSERT_EQ(0, dtor_called);
        }
        ASSERT_EQ(1, dtor_called);
    }

    TEST(maybe_owning, Reset) {
        dtor_called = 0;
        maybe_owning<C> ppp(std::make_unique<C>());
        ASSERT_EQ(0, dtor_called);
        ppp.reset();
        ASSERT_EQ(1, dtor_called);
    }

    TEST(maybe_owning, TransferringOwnership) {
        dtor_called = 0;
        maybe_owning<C> ppp(std::make_unique<C>());
        ASSERT_EQ(0, dtor_called);
        ppp = std::make_unique<C>();
        ASSERT_EQ(1, dtor_called);
    }

// Shouldn't leak.
    TEST(maybe_owning, MoveIntoNonNull) {
        dtor_called = 0;
        {
            maybe_owning<C> ppp(std::make_unique<C>());
            maybe_owning<C> ppp2(std::make_unique<C>());
            ASSERT_EQ(0, dtor_called);
            ppp = std::move(ppp2);
            ASSERT_EQ(1, dtor_called);
        }
    }

    TEST(maybe_owning, SelfMove) {
        dtor_called = 0;
        {
            maybe_owning<C> ppp(std::make_unique<C>());
            ASSERT_EQ(0, dtor_called);
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
            ppp = std::move(ppp);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
            ASSERT_EQ(0, dtor_called);
        }
        ASSERT_EQ(1, dtor_called);
    }

    TEST(maybe_owning, Conversion) {
        maybe_owning<C> ppp(new C(), true);
        maybe_owning<const C> ppp2(std::move(ppp));
        ASSERT_TRUE(!!ppp2);
        ASSERT_FALSE(!!ppp);
    }

    TEST(maybe_owning, Conversion2) {
        maybe_owning<C> ppp(new C(), true);
        maybe_owning<const C> ppp2;
        ppp2 = std::move(ppp);
        ASSERT_TRUE(!!ppp2);
        ASSERT_FALSE(!!ppp);
    }

    TEST(maybe_owning, ConversionUniquePtr) {
        auto ppp = std::make_unique<C>();
        maybe_owning<const C> ppp2(std::move(ppp));
        ASSERT_TRUE(!!ppp2);
        ASSERT_FALSE(!!ppp);
    }

    TEST(maybe_owning, ConversionUniquePtr2) {
        auto ppp = std::make_unique<C>();
        maybe_owning<const C> ppp2;
        ppp2 = std::move(ppp);
        ASSERT_TRUE(!!ppp2);
        ASSERT_FALSE(!!ppp);
    }

    TEST(maybe_owning, DeductionGuides) {
        maybe_owning ppp(new C(), true);
        ASSERT_TRUE(!!ppp);
    }

    TEST(maybe_owning_argument, All) {
        int x = 0;
        AcceptMaybeOwningArgument(&x);
        AcceptMaybeOwningArgument(std::make_unique<int>());
        AcceptMaybeOwningArgument(nullptr);

        Derived derived;
        AcceptMaybeOwningArgumentBase(&derived);

        static_assert(std::is_convertible_v<Derived *, maybe_owning_argument<Base>>);
    }

}  // namespace flare
