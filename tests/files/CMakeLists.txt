

carbin_cc_test(
        NAME
        multi1_test
        SOURCES
        "multi1_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        multi2_test
        SOURCES
        "multi2_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        impl_test
        SOURCES
        "impl_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        fs_exception_test
        SOURCES
        "fs_exception_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        filesystem_test
        SOURCES
        "filesystem_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)