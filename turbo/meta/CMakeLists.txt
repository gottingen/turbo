#
# Copyright 2017 The Turbo Authors.
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



turbo_cc_test(
  NAME
    type_traits_test
  SRCS
    "type_traits_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)




turbo_cc_test(
        NAME
        utility_test
        SRCS
        "utility_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)



turbo_cc_test(
        NAME
        span_test
        SRCS
        "span_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::exception_testing
        turbo::hash_testing
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        span_test_noexceptions
        SRCS
        "span_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::exception_testing
        turbo::hash_testing
        turbo::turbo
        GTest::gmock_main
)


# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        conformance_testing
        HDRS
        "internal/conformance_aliases.h"
        "internal/conformance_archetype.h"
        "internal/conformance_profile.h"
        "internal/conformance_testing.h"
        "internal/conformance_testing_helpers.h"
        "internal/parentheses.h"
        "internal/transform_args.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
        TESTONLY
)

turbo_cc_test(
        NAME
        conformance_testing_test
        SRCS
        "internal/conformance_testing_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        ${TURBO_EXCEPTIONS_FLAG}
        LINKOPTS
        ${TURBO_EXCEPTIONS_FLAG_LINKOPTS}
        DEPS
        turbo::conformance_testing
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        conformance_testing_test_no_exceptions
        SRCS
        "internal/conformance_testing_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::conformance_testing
        turbo::turbo
        GTest::gmock_main
)



turbo_cc_test(
        NAME
        compare_test
        SRCS
        "compare_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)


turbo_cc_test(
        NAME
        algorithm_test
        SRCS
        "algorithm_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)


turbo_cc_test(
        NAME
        container_test
        SRCS
        "container_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)


turbo_cc_test(
        NAME
        any_invocable_test
        SRCS
        "any_invocable_test.cc"
        "internal/any_invocable.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)


turbo_cc_test(
        NAME
        bind_front_test
        SRCS
        "bind_front_test.cc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        function_ref_test
        SRCS
        "function_ref_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        turbo::test_instance_tracker
        GTest::gmock_main
)
