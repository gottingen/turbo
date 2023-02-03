
add_subdirectory(test)

turbo_cc_test(
        NAME
        compositions_test
        SRCS
        "compositions_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)
