
file(GLOB REF_SRC "*.cc")
turbo_cc_library(
        NAME
        utf_ref
        SRCS
        ${REF_SRC}
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
        TESTONLY
)
