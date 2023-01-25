#
# Copyright 2020 The Turbo Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
turbo_cc_library(
  NAME
    status
  HDRS
    "status.h"
  SRCS
    "internal/status_internal.h"
    "status.cc"
    "status_payload_printer.h"
    "status_payload_printer.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::atomic_hook
    turbo::config
    turbo::cord
    turbo::core_headers
    turbo::function_ref
    turbo::inlined_vector
    turbo::optional
    turbo::raw_logging_internal
    turbo::stacktrace
    turbo::str_format
    turbo::strerror
    turbo::strings
    turbo::symbolize
  PUBLIC
)

turbo_cc_test(
  NAME
    status_test
  SRCS
   "status_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::status
    turbo::strings
    GTest::gmock_main
)

turbo_cc_library(
  NAME
    statusor
  HDRS
    "statusor.h"
  SRCS
    "statusor.cc"
    "internal/statusor_internal.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::status
    turbo::core_headers
    turbo::raw_logging_internal
    turbo::type_traits
    turbo::strings
    turbo::utility
    turbo::variant
  PUBLIC
)

turbo_cc_test(
  NAME
    statusor_test
  SRCS
   "statusor_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::status
    turbo::statusor
    GTest::gmock_main
)



turbo_cc_library(
        NAME
        bits
        HDRS
        "bits.h"
        "internal/bits.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::core_headers
        PUBLIC
)

turbo_cc_test(
        NAME
        bits_test
        SRCS
        "bits_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::bits
        turbo::core_headers
        turbo::random_random
        GTest::gmock_main
)

turbo_cc_library(
        NAME
        int128
        HDRS
        "int128.h"
        SRCS
        "int128.cc"
        "int128_have_intrinsic.inc"
        "int128_no_intrinsic.inc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::config
        turbo::core_headers
        turbo::bits
        PUBLIC
)

turbo_cc_test(
        NAME
        int128_test
        SRCS
        "int128_stream_test.cc"
        "int128_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::int128
        turbo::base
        turbo::hash_testing
        turbo::type_traits
        GTest::gmock_main
)

# component target
turbo_cc_library(
        NAME
        numeric
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::int128
        PUBLIC
)

turbo_cc_library(
        NAME
        numeric_representation
        HDRS
        "internal/representation.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::config
        PUBLIC
)

turbo_cc_library(
        NAME
        log_severity
        HDRS
        "log_severity.h"
        SRCS
        "log_severity.cc"
        DEPS
        turbo::core_headers
        COPTS
        ${TURBO_DEFAULT_COPTS}
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        raw_logging_internal
        HDRS
        "internal/raw_logging.h"
        SRCS
        "internal/raw_logging.cc"
        DEPS
        turbo::atomic_hook
        turbo::config
        turbo::core_headers
        turbo::errno_saver
        turbo::log_severity
        COPTS
        ${TURBO_DEFAULT_COPTS}
)


# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        base_internal
        HDRS
        "internal/hide_ptr.h"
        "internal/identity.h"
        "internal/inline_variable.h"
        "internal/invoke.h"
        "internal/scheduling_mode.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::config
        turbo::type_traits
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        exception_testing
        HDRS
        "internal/exception_testing.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::config
        GTest::gtest
        TESTONLY
)


# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        exception_safety_testing
        HDRS
        "internal/exception_safety_testing.h"
        SRCS
        "internal/exception_safety_testing.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::config
        turbo::pretty_function
        turbo::memory
        turbo::meta
        turbo::strings
        turbo::utility
        GTest::gtest
        TESTONLY
)


turbo_cc_test(
        NAME
        turbo_exception_safety_testing_test
        SRCS
        "exception_safety_testing_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::exception_safety_testing
        turbo::memory
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        bit_cast_test
        SRCS
        "bit_cast_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::base
        turbo::core_headers
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        inline_variable_test
        SRCS
        "internal/inline_variable_testing.h"
        "inline_variable_test.cc"
        "inline_variable_test_a.cc"
        "inline_variable_test_b.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::base_internal
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        invoke_test
        SRCS
        "invoke_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::base_internal
        turbo::memory
        turbo::strings
        GTest::gmock
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        raw_logging_test
        SRCS
        "raw_logging_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::raw_logging_internal
        turbo::strings
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        log_severity_test
        SRCS
        "log_severity_test.cc"
        DEPS
        turbo::flags_internal
        turbo::flags_marshalling
        turbo::log_severity
        turbo::strings
        GTest::gmock
        GTest::gtest_main
)


# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        fast_type_id
        HDRS
        "internal/fast_type_id.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::config
)

turbo_cc_test(
        NAME
        fast_type_id_test
        SRCS
        "internal/fast_type_id_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::fast_type_id
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        strerror
        SRCS
        "internal/strerror.cc"
        HDRS
        "internal/strerror.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::config
        turbo::core_headers
        turbo::errno_saver
)

turbo_cc_test(
        NAME
        strerror_test
        SRCS
        "internal/strerror_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::strerror
        turbo::strings
        GTest::gmock
        GTest::gtest_main
)
