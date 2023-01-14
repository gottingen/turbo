
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "testing/sstream_workaround.h"
#include "turbo/memory/ref_ptr.h"
#include <atomic>
#include <chrono>
#include <thread>
#include "turbo/base/fast_rand.h"
#include "testing/gtest_wrap.h"

using namespace std::literals;

namespace turbo {

    struct RefCounted1 {
        std::atomic<int> ref_count{1};

        RefCounted1() { ++instances; }

        ~RefCounted1() { --instances; }

        int xxx = 12345;
        inline static int instances = 0;
    };

    template<>
    struct ref_traits<RefCounted1> {
        static void reference(RefCounted1 *rc) {
            rc->ref_count.fetch_add(1, std::memory_order_relaxed);
        }

        static void dereference(RefCounted1 *rc) {
            if (rc->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                delete rc;
            }
        }
    };

    struct RefCounted2 : ref_counted<RefCounted2> {
        RefCounted2() { ++instances; }

        ~RefCounted2() { --instances; }

        inline static int instances = 0;
    };

    struct RefCountedVirtual : ref_counted<RefCountedVirtual> {
        RefCountedVirtual() { ++instances; }

        virtual ~RefCountedVirtual() { --instances; }

        inline static int instances = 0;
    };

    struct RefCounted3 : RefCountedVirtual {
        RefCounted3() { ++instances; }

        ~RefCounted3() { --instances; }

        inline static int instances = 0;
    };

    static_assert(!memory_internal::is_ref_counted_directly_v<RefCounted3>);
    static_assert(memory_internal::is_ref_counted_indirectly_safe_v<RefCounted3>);

    TEST(ref_ptr, ReferenceCount) {
        {
            auto ptr = new RefCounted1();
            ptr->ref_count = 0;
            ASSERT_EQ(1, RefCounted1::instances);
            ref_ptr p1(ref_ptr_v, ptr);
            ASSERT_EQ(1, ptr->ref_count);
            {
                ref_ptr p2(p1);
                ASSERT_EQ(2, ptr->ref_count);
                ref_ptr p3(std::move(p2));
                ASSERT_EQ(2, ptr->ref_count);
            }
            {
                ref_ptr p2(p1);
                ASSERT_EQ(2, ptr->ref_count);
                p2.reset();
                ASSERT_EQ(1, ptr->ref_count);
            }
            {
                ref_ptr p2(p1);
                ASSERT_EQ(2, ptr->ref_count);
                auto ptr = p2.leak();
                ASSERT_EQ(2, ptr->ref_count);
                ref_ptr p3(adopt_ptr_v, ptr);
                ASSERT_EQ(2, ptr->ref_count);
            }
            ASSERT_EQ(1, ptr->ref_count);
        }
        ASSERT_EQ(0, RefCounted1::instances);
    }

    TEST(ref_ptr, ref_counted) {
        {
            auto ptr = new RefCounted2();
            ASSERT_EQ(1, RefCounted2::instances);
            ref_ptr p1(adopt_ptr_v, ptr);
        }
        ASSERT_EQ(0, RefCounted2::instances);
    }

    TEST(ref_ptr, RefCountedVirtualDtor) {
        {
            auto ptr = new RefCounted3();
            ASSERT_EQ(1, RefCounted3::instances);
            ASSERT_EQ(1, RefCountedVirtual::instances);
            ref_ptr p1(adopt_ptr_v, ptr);
        }
        ASSERT_EQ(0, RefCounted3::instances);
    }

    TEST(ref_ptr, ImplicitlyCast) {
        {
            ref_ptr ptr = make_ref_counted<RefCounted3>();
            ASSERT_EQ(1, RefCounted3::instances);
            ASSERT_EQ(1, RefCountedVirtual::instances);
            ref_ptr <RefCountedVirtual> p1(ptr);
            ASSERT_EQ(1, RefCounted3::instances);
            ASSERT_EQ(1, RefCountedVirtual::instances);
            ref_ptr <RefCountedVirtual> p2(std::move(ptr));
            ASSERT_EQ(1, RefCounted3::instances);
            ASSERT_EQ(1, RefCountedVirtual::instances);
        }
        ASSERT_EQ(0, RefCounted3::instances);
        ASSERT_EQ(0, RefCountedVirtual::instances);
    }

    TEST(ref_ptr, CopyFromNull) {
        ref_ptr <RefCounted1> p1, p2;
        p1 = p2;
        // Shouldn't crash.
    }

    TEST(ref_ptr, MoveFromNull) {
        ref_ptr <RefCounted1> p1, p2;
        p1 = std::move(p2);
        // Shouldn't crash.
    }

    TEST(ref_ptr, AtomicOps) {
        std::atomic<ref_ptr < RefCounted1>>
        atomic{nullptr};

        EXPECT_EQ(0, RefCounted1::instances);
        EXPECT_EQ(nullptr, atomic.load());
        EXPECT_EQ(0, RefCounted1::instances);
        auto p1 = make_ref_counted<RefCounted1>();
        EXPECT_EQ(1, RefCounted1::instances);
        atomic.store(p1);
        EXPECT_EQ(p1.get(), atomic.load().get());
        auto p2 = make_ref_counted<RefCounted1>();
        EXPECT_EQ(2, RefCounted1::instances);
        EXPECT_EQ(p1.get(), atomic.exchange(p2).get());
        EXPECT_EQ(2, RefCounted1::instances);
        p1.reset();
        EXPECT_EQ(1, RefCounted1::instances);
        EXPECT_FALSE(atomic.compare_exchange_strong(p1, p2));
        EXPECT_TRUE(atomic.compare_exchange_weak(p2, p2));
        EXPECT_EQ(1, RefCounted1::instances);
        EXPECT_TRUE(
                atomic.compare_exchange_strong(p2, make_ref_counted<RefCounted1>()));
        EXPECT_EQ(2, RefCounted1::instances);
        EXPECT_EQ(12345, static_cast<ref_ptr <RefCounted1>>(atomic)->xxx);
    }

    // Heap check should helps with us here to check if any leak occurs.
    /*
    TEST(ref_ptr, AtomicDontLeak) {
        ref_ptr <RefCounted1> ps[2] = {nullptr, make_ref_counted<RefCounted1>()};
        ref_ptr <RefCounted1> temps[10];

        ASSERT_EQ(1, RefCounted1::instances);
        for (auto &&e : temps) {
            e = make_ref_counted<RefCounted1>();
        }
        ASSERT_EQ(11, RefCounted1::instances);

        for (auto &&from : ps) {
            std::thread ts[10];
            std::atomic<ref_ptr < RefCounted1>> atomic{from};
            std::atomic<bool> ever_success{false};

            ASSERT_EQ(11, RefCounted1::instances);
            for (auto &&e : ts) {
                e = std::thread([&] {
                    auto op = turbo::base::fast_rand() % 4;
                    while (!ever_success) {
                        if (op == 0) {
                            atomic.store(temps[turbo::base::fast_rand_less_than(9)], std::memory_order_release);
                        } else if (op == 1) {
                            if (auto ptr = atomic.load(std::memory_order_acquire)) {
                                ASSERT_EQ(12345, ptr->xxx);
                            }
                        } else if (op == 2) {
                            auto p1 = temps[0], p2 = temps[1], p3 = temps[2], p4 = temps[3];
                            if (atomic.compare_exchange_strong(p1, temps[1],
                                                               std::memory_order_acq_rel,
                                                               std::memory_order_acquire)) {
                                ever_success = true;
                            }
                            if (atomic.compare_exchange_weak(p2, temps[2],
                                                             std::memory_order_acq_rel,
                                                             std::memory_order_acquire)) {
                                ever_success = true;
                            }
                            if (atomic.compare_exchange_strong(p3, temps[3],
                                                               std::memory_order_acq_rel)) {
                                ever_success = true;
                            }
                            if (atomic.compare_exchange_weak(p4, temps[4],
                                                             std::memory_order_acq_rel)) {
                                ever_success = true;
                            }
                        } else {
                            atomic.exchange(temps[turbo::base::fast_rand_less_than(9)]);
                        }
                        ASSERT_EQ(11, RefCounted1::instances);
                    }
                });
            }
            for (auto &&e : ts) {
                e.join();
            }
            EXPECT_TRUE(ever_success.load());
        }
    }
*/
}  // namespace turbo
