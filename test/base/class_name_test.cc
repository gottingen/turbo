
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include "flare/base/class_name.h"
#include "flare/log/logging.h"

namespace flare::base {
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
        ASSERT_EQ("add_something", flare::base::demangle("add_something"));
        ASSERT_EQ("dp::FiberPBCommand<proto::PbRouteTable, proto::PbRouteAck>::marshal(dp::ParamWriter*)::__FUNCTION__",
                  flare::base::demangle(
                          "_ZZN2dp14FiberPBCommandIN5proto12PbRouteTableENS1_10PbRouteAckEE7marshalEPNS_11ParamWriterEE12__FUNCTION__"));
        ASSERT_EQ("7&8", flare::base::demangle("7&8"));
    }

    TEST_F(ClassNameTest, class_name_sanity) {
        ASSERT_EQ("char", flare::base::class_name_str('\0'));
        ASSERT_STREQ("short", flare::base::class_name<short>());
        ASSERT_EQ("long", flare::base::class_name_str(1L));
        ASSERT_EQ("unsigned long", flare::base::class_name_str(1UL));
        ASSERT_EQ("float", flare::base::class_name_str(1.1f));
        ASSERT_EQ("double", flare::base::class_name_str(1.1));
        ASSERT_STREQ("char*", flare::base::class_name<char *>());
        ASSERT_STREQ("char const*", flare::base::class_name<const char *>());
        ASSERT_STREQ("flare::base::foobar::MyClass", flare::base::class_name<flare::base::foobar::MyClass>());

        int array[32];
        ASSERT_EQ("int [32]", flare::base::class_name_str(array));

        FLARE_LOG(INFO) << flare::base::class_name_str(this);
        FLARE_LOG(INFO) << flare::base::class_name_str(*this);
    }
}
