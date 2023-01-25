#
# Copyright 2017 The Turbo Authors.
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
turbo_cc_library(
  NAME
    atomic_hook
  HDRS
    "internal/atomic_hook.h"
  DEPS
    turbo::config
    turbo::core_headers
  COPTS
    ${TURBO_DEFAULT_COPTS}
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    errno_saver
  HDRS
    "internal/errno_saver.h"
  DEPS
    turbo::config
  COPTS
    ${TURBO_DEFAULT_COPTS}
)



# Internal-only target, do not depend on directly.
turbo_cc_library(
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
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base_internal
    turbo::core_headers
    turbo::errno_saver
)

turbo_cc_library(
  NAME
    config
  HDRS
    "config.h"
    "options.h"
    "policy_checks.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  PUBLIC
)

turbo_cc_library(
  NAME
    dynamic_annotations
  HDRS
    "dynamic_annotations.h"
  SRCS
    "internal/dynamic_annotations.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::config
  PUBLIC
)

turbo_cc_library(
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
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::config
  PUBLIC
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    malloc_internal
  HDRS
    "internal/direct_mmap.h"
    "internal/low_level_alloc.h"
  SRCS
    "internal/low_level_alloc.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::dynamic_annotations
    turbo::raw_logging_internal
    Threads::Threads
)

turbo_cc_library(
  NAME
    base
  HDRS
    "call_once.h"
    "../base/casts.h"
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
    ${TURBO_DEFAULT_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
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
turbo_cc_library(
  NAME
    throw_delegate
  HDRS
    "internal/throw_delegate.h"
  SRCS
    "internal/throw_delegate.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::raw_logging_internal
)



# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    pretty_function
  HDRS
    "internal/pretty_function.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
)


# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    atomic_hook_test_helper
  SRCS
    "internal/atomic_hook_test_helper.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::atomic_hook
    turbo::core_headers
  TESTONLY
)

turbo_cc_test(
  NAME
    atomic_hook_test
  SRCS
    "internal/atomic_hook_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::atomic_hook_test_helper
    turbo::atomic_hook
    turbo::core_headers
    GTest::gmock
    GTest::gtest_main
)


turbo_cc_test(
  NAME
    errno_saver_test
  SRCS
    "internal/errno_saver_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::errno_saver
    turbo::strerror
    GTest::gmock
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    throw_delegate_test
  SRCS
    "throw_delegate_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::base
    turbo::config
    turbo::throw_delegate
    GTest::gtest_main
)




# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    spinlock_test_common
  SRCS
    "spinlock_test_common.cc"
  COPTS
    ${TURBO_TEST_COPTS}
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
turbo_cc_test(
  NAME
    spinlock_test
  SRCS
    "spinlock_test_common.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::base
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::synchronization
    GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    endian
  HDRS
    "internal/endian.h"
    "internal/unaligned_access.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::config
    turbo::core_headers
  PUBLIC
)

turbo_cc_test(
  NAME
    endian_test
  SRCS
    "internal/endian_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::base
    turbo::config
    turbo::endian
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    config_test
  SRCS
    "config_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::config
    turbo::synchronization
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    call_once_test
  SRCS
    "call_once_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::base
    turbo::core_headers
    turbo::synchronization
    GTest::gtest_main
)


turbo_cc_test(
  NAME
    sysinfo_test
  SRCS
    "internal/sysinfo_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::base
    turbo::synchronization
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    low_level_alloc_test
  SRCS
    "internal/low_level_alloc_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::malloc_internal
    turbo::node_hash_map
    Threads::Threads
)

turbo_cc_test(
  NAME
    thread_identity_test
  SRCS
    "internal/thread_identity_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::base
    turbo::core_headers
    turbo::synchronization
    Threads::Threads
    GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    scoped_set_env
  SRCS
    "internal/scoped_set_env.cc"
  HDRS
    "internal/scoped_set_env.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::raw_logging_internal
)

turbo_cc_test(
  NAME
    scoped_set_env_test
  SRCS
    "internal/scoped_set_env_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::scoped_set_env
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    cmake_thread_test
  SRCS
    "internal/cmake_thread_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::base
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

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    prefetch
  HDRS
    "internal/prefetch.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::config
)

turbo_cc_test(
  NAME
    prefetch_test
  SRCS
    "internal/prefetch_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::prefetch
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    optimization_test
  SRCS
    "optimization_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::core_headers
    turbo::optional
    GTest::gtest_main
)
