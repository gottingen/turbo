

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"

#include "turbo/memory/allocator.h"

// SchedulerParams holds Scheduler construction parameters for testing.
struct SchedulerParams {
    int numWorkerThreads;

    friend std::ostream &operator<<(std::ostream &os,
                                    const SchedulerParams &params) {
        return os << "SchedulerParams{"
                  << "numWorkerThreads: " << params.numWorkerThreads << "}";
    }
};

// with_tracked_allocator is a test fixture that does not bind a scheduler.
class with_tracked_allocator : public testing::Test {
public:
    void SetUp() override {
        allocator = new turbo::tracked_allocator(turbo::allocator::Default);
    }

    void TearDown() override {
        auto stats = allocator->stats();
        ASSERT_EQ(stats.numAllocations(), 0U);
        ASSERT_EQ(stats.bytesAllocated(), 0U);
        delete allocator;
    }

    turbo::tracked_allocator *allocator = nullptr;
};
