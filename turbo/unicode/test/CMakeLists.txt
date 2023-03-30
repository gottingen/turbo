


# Internal-only target, do not depend on directly.
file(GLOB REF_SRC "reference/*.cc")
turbo_cc_library(
        NAME
        test_reference
        SRCS
        ${REF_SRC}
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        TESTONLY
)

# Internal-only target, do not depend on directly.
file(GLOB HELPER_SRC "helpers/*.cc")
turbo_cc_library(
        NAME
        test_helper
        SRCS
        ${HELPER_SRC}
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        TESTONLY
)

file(GLOB UNICODE_TEST_SRCS "*.cc")
foreach(UNICODE_UT ${UNICODE_TEST_SRCS})
    get_filename_component(UNICODE_UT_WE ${UNICODE_UT} NAME_WE)
    turbo_cc_test(
            NAME
            ${UNICODE_UT_WE}
            SRCS
            ${UNICODE_UT}
            COPTS
            ${TURBO_TEST_COPTS}
            LINKOPTS
            ${TURBO_DEFAULT_LINKOPTS}
            DEPS
            turbo::test_reference
            turbo::test_helper
            turbo::turbo
            GTest::gmock
            GTest::gtest_main
    )
endforeach()