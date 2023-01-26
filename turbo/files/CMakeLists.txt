

turbo_cc_test(
        NAME
        multi1_test
        SRCS
        "multi1_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        multi2_test
        SRCS
        "multi2_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        impl_test
        SRCS
        "impl_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        fs_exception_test
        SRCS
        "fs_exception_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        filesystem_test
        SRCS
        "filesystem_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)