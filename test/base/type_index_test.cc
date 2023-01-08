
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#include "flare/base/type_index.h"
#include <string>
#include "testing/gtest_wrap.h"

namespace flare::base {

    TEST(type_index, Compare) {
        constexpr type_index empty1, empty2;

        ASSERT_EQ(empty1, empty2);

        // Statically initializable.
        constexpr auto str_type = get_type_index<std::string>();
        constexpr auto int_type = get_type_index<int>();

        // `operator !=` is not implemented, we can't use `ASSERT_NE` here.
        ASSERT_FALSE(empty1 == str_type);
        ASSERT_FALSE(empty1 == int_type);
        ASSERT_FALSE(str_type == int_type);

        if (str_type < int_type) {
            ASSERT_FALSE(int_type < str_type);
        } else {
            ASSERT_FALSE(str_type < int_type);
        }
    }

    TEST(type_index, TypeIndexOfRuntime) {
        constexpr auto str_type = get_type_index<std::string>();
        ASSERT_EQ(std::type_index(typeid(std::string)),
                  str_type.get_runtime_type_index());
    }

}  // namespace flare
