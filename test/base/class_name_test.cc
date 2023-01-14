
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include "turbo/base/class_name.h"
#include "turbo/log/logging.h"

namespace turbo::base {
    namespace foobar {
        struct MyClass {
        };
    }
}

namespace {
    class ClassNameTest : public ::testing::Test {
    protected:
        virtual void SetUp() {
            srand(time(0));
        };
    };

    TEST_F(ClassNameTest, demangle) {
        ASSERT_EQ("add_something", turbo::base::demangle("add_something"));
        ASSERT_EQ("dp::FiberPBCommand<proto::PbRouteTable, proto::PbRouteAck>::marshal(dp::ParamWriter*)::__FUNCTION__",
                  turbo::base::demangle(
                          "_ZZN2dp14FiberPBCommandIN5proto12PbRouteTableENS1_10PbRouteAckEE7marshalEPNS_11ParamWriterEE12__FUNCTION__"));
        ASSERT_EQ("7&8", turbo::base::demangle("7&8"));
    }

    TEST_F(ClassNameTest, class_name_sanity) {
        ASSERT_EQ("char", turbo::base::class_name_str('\0'));
        ASSERT_STREQ("short", turbo::base::class_name<short>());
        ASSERT_EQ("long", turbo::base::class_name_str(1L));
        ASSERT_EQ("unsigned long", turbo::base::class_name_str(1UL));
        ASSERT_EQ("float", turbo::base::class_name_str(1.1f));
        ASSERT_EQ("double", turbo::base::class_name_str(1.1));
        ASSERT_STREQ("char*", turbo::base::class_name<char *>());
        ASSERT_STREQ("char const*", turbo::base::class_name<const char *>());
        ASSERT_STREQ("turbo::base::foobar::MyClass", turbo::base::class_name<turbo::base::foobar::MyClass>());

        int array[32];
        ASSERT_EQ("int [32]", turbo::base::class_name_str(array));

        TURBO_LOG(INFO) << turbo::base::class_name_str(this);
        TURBO_LOG(INFO) << turbo::base::class_name_str(*this);
    }
}
