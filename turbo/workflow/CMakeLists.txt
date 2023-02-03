
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

turbo_cc_test(
        NAME
        asyncs_test
        SRCS
        "asyncs_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        workers_test
        SRCS
        "workers_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        sorting_test
        SRCS
        "sorting_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        movable_test
        SRCS
        "movable_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        control_flow_test
        SRCS
        "control_flow_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        priorities_test
        SRCS
        "priorities_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)