

turbo_cc_library(
        NAME
        utf_helper
        SRCS
        "random_int.cc"
        "random_utf8.cc"
        "random_utf16.cc"
        "random_utf32.cc"
        "transcode_test_base.cc"
        "test.cc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::utf_ref
        turbo::turbo
        TESTONLY
)