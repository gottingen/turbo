
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/memory/resident.h"
#include "testing/gtest_wrap.h"

namespace flare {

    struct C {
        C() { ++instances; }

        ~C() { --instances; }

        inline static std::size_t instances{};
    };

    struct D {
        void foo() {
            [[maybe_unused]] static resident_singleton<D> test_compilation2;
        }
    };

    resident<int> test_compilation2;

    TEST(resident, All) {
        ASSERT_EQ(0ul, C::instances);
        {
            C c1;
            ASSERT_EQ(1ul, C::instances);
            [[maybe_unused]] resident<C> c2;
            ASSERT_EQ(2ul, C::instances);
        }
        // Not 0, as `resident<C>` is not destroyed.
        ASSERT_EQ(1ul, C::instances);
    }

}  // namespace flare
