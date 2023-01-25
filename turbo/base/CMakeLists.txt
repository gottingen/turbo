#
# Copyright 2017 The Abseil Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

find_library(LIBRT rt)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    atomic_hook
  HDRS
    "internal/atomic_hook.h"
  DEPS
    turbo::config
    turbo::core_headers
  COPTS
    ${ABSL_DEFAULT_COPTS}
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    errno_saver
  HDRS
    "internal/errno_saver.h"
  DEPS
    turbo::config
  COPTS
    ${ABSL_DEFAULT_COPTS}
)

absl_cc_library(
  NAME
    log_severity
  HDRS
    "log_severity.h"
  SRCS
    "log_severity.cc"
  DEPS
    turbo::core_headers
  COPTS
    ${ABSL_DEFAULT_COPTS}
)

# Internal-only target, do not depend on directly.
absl_cc_library(
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
    ${ABSL_DEFAULT_COPTS}
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    spinlock_wait
  HDRS
    "internal/spinlock_wait.h"
  SRCS
    "internal/spinlock_akaros.inc"
    "internal/spinlock_linux.inc"
    "internal/spinlock_posix.inc"
    "internal/spinlock_wait.cc"
    "internal/spinlock_win32.inc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::base_internal
    turbo::core_headers
    turbo::errno_saver
)

absl_cc_library(
  NAME
    config
  HDRS
    "config.h"
    "options.h"
    "policy_checks.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  PUBLIC
)

absl_cc_library(
  NAME
    dynamic_annotations
  HDRS
    "dynamic_annotations.h"
  SRCS
    "internal/dynamic_annotations.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
  PUBLIC
)

absl_cc_library(
  NAME
    core_headers
  HDRS
    "attributes.h"
    "const_init.h"
    "macros.h"
    "optimization.h"
    "port.h"
    "thread_annotations.h"
    "internal/thread_annotations.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
  PUBLIC
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    malloc_internal
  HDRS
    "internal/direct_mmap.h"
    "internal/low_level_alloc.h"
  SRCS
    "internal/low_level_alloc.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::dynamic_annotations
    turbo::raw_logging_internal
    Threads::Threads
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    base_internal
  HDRS
    "internal/hide_ptr.h"
    "internal/identity.h"
    "internal/inline_variable.h"
    "internal/invoke.h"
    "internal/scheduling_mode.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::type_traits
)

absl_cc_library(
  NAME
    base
  HDRS
    "call_once.h"
    "casts.h"
    "internal/cycleclock.h"
    "internal/cycleclock_config.h"
    "internal/low_level_scheduling.h"
    "internal/per_thread_tls.h"
    "internal/spinlock.h"
    "internal/sysinfo.h"
    "internal/thread_identity.h"
    "internal/tsan_mutex_interface.h"
    "internal/unscaledcycleclock.h"
    "internal/unscaledcycleclock_config.h"
  SRCS
    "internal/cycleclock.cc"
    "internal/spinlock.cc"
    "internal/sysinfo.cc"
    "internal/thread_identity.cc"
    "internal/unscaledcycleclock.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
    $<$<BOOL:${LIBRT}>:-lrt>
    $<$<BOOL:${MINGW}>:-ladvapi32>
  DEPS
    turbo::atomic_hook
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::dynamic_annotations
    turbo::log_severity
    turbo::raw_logging_internal
    turbo::spinlock_wait
    turbo::type_traits
    Threads::Threads
  PUBLIC
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    throw_delegate
  HDRS
    "internal/throw_delegate.h"
  SRCS
    "internal/throw_delegate.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::raw_logging_internal
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    exception_testing
  HDRS
    "internal/exception_testing.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    GTest::gtest
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    pretty_function
  HDRS
    "internal/pretty_function.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    exception_safety_testing
  HDRS
    "internal/exception_safety_testing.h"
  SRCS
    "internal/exception_safety_testing.cc"
  COPTS
    ${ABSL_TEST_COPTS}
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

absl_cc_test(
  NAME
    absl_exception_safety_testing_test
  SRCS
    "exception_safety_testing_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::exception_safety_testing
    turbo::memory
    GTest::gtest_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    atomic_hook_test_helper
  SRCS
    "internal/atomic_hook_test_helper.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::atomic_hook
    turbo::core_headers
  TESTONLY
)

absl_cc_test(
  NAME
    atomic_hook_test
  SRCS
    "internal/atomic_hook_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::atomic_hook_test_helper
    turbo::atomic_hook
    turbo::core_headers
    GTest::gmock
    GTest::gtest_main
)

absl_cc_test(
  NAME
    bit_cast_test
  SRCS
    "bit_cast_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
    turbo::core_headers
    GTest::gtest_main
)

absl_cc_test(
  NAME
    errno_saver_test
  SRCS
    "internal/errno_saver_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::errno_saver
    turbo::strerror
    GTest::gmock
    GTest::gtest_main
)

absl_cc_test(
  NAME
    throw_delegate_test
  SRCS
    "throw_delegate_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
    turbo::config
    turbo::throw_delegate
    GTest::gtest_main
)

absl_cc_test(
  NAME
    inline_variable_test
  SRCS
    "internal/inline_variable_testing.h"
    "inline_variable_test.cc"
    "inline_variable_test_a.cc"
    "inline_variable_test_b.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base_internal
    GTest::gtest_main
)

absl_cc_test(
  NAME
    invoke_test
  SRCS
    "invoke_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base_internal
    turbo::memory
    turbo::strings
    GTest::gmock
    GTest::gtest_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    spinlock_test_common
  SRCS
    "spinlock_test_common.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
    turbo::config
    turbo::base_internal
    turbo::core_headers
    turbo::synchronization
    GTest::gtest
  TESTONLY
)

# On bazel BUILD this target use "alwayslink = 1" which is not implemented here
absl_cc_test(
  NAME
    spinlock_test
  SRCS
    "spinlock_test_common.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::synchronization
    GTest::gtest_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    endian
  HDRS
    "internal/endian.h"
    "internal/unaligned_access.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::config
    turbo::core_headers
  PUBLIC
)

absl_cc_test(
  NAME
    endian_test
  SRCS
    "internal/endian_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
    turbo::config
    turbo::endian
    GTest::gtest_main
)

absl_cc_test(
  NAME
    config_test
  SRCS
    "config_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::config
    turbo::synchronization
    GTest::gtest_main
)

absl_cc_test(
  NAME
    call_once_test
  SRCS
    "call_once_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
    turbo::core_headers
    turbo::synchronization
    GTest::gtest_main
)

absl_cc_test(
  NAME
    raw_logging_test
  SRCS
    "raw_logging_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::raw_logging_internal
    turbo::strings
    GTest::gtest_main
)

absl_cc_test(
  NAME
    sysinfo_test
  SRCS
    "internal/sysinfo_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
    turbo::synchronization
    GTest::gtest_main
)

absl_cc_test(
  NAME
    low_level_alloc_test
  SRCS
    "internal/low_level_alloc_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::malloc_internal
    turbo::node_hash_map
    Threads::Threads
)

absl_cc_test(
  NAME
    thread_identity_test
  SRCS
    "internal/thread_identity_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
    turbo::core_headers
    turbo::synchronization
    Threads::Threads
    GTest::gtest_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    scoped_set_env
  SRCS
    "internal/scoped_set_env.cc"
  HDRS
    "internal/scoped_set_env.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::raw_logging_internal
)

absl_cc_test(
  NAME
    scoped_set_env_test
  SRCS
    "internal/scoped_set_env_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::scoped_set_env
    GTest::gtest_main
)

absl_cc_test(
  NAME
    cmake_thread_test
  SRCS
    "internal/cmake_thread_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::base
)

absl_cc_test(
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
absl_cc_library(
  NAME
    strerror
  SRCS
    "internal/strerror.cc"
  HDRS
    "internal/strerror.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    turbo::config
    turbo::core_headers
    turbo::errno_saver
)

absl_cc_test(
  NAME
    strerror_test
  SRCS
    "internal/strerror_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::strerror
    turbo::strings
    GTest::gmock
    GTest::gtest_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    fast_type_id
  HDRS
    "internal/fast_type_id.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    turbo::config
)

absl_cc_test(
  NAME
    fast_type_id_test
  SRCS
    "internal/fast_type_id_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::fast_type_id
    GTest::gtest_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    prefetch
  HDRS
    "internal/prefetch.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    turbo::config
)

absl_cc_test(
  NAME
    prefetch_test
  SRCS
    "internal/prefetch_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::prefetch
    GTest::gtest_main
)

absl_cc_test(
  NAME
    optimization_test
  SRCS
    "optimization_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::core_headers
    turbo::optional
    GTest::gtest_main
)
