#
# Copyright 2017 The Abseil Authors.
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

find_library(EXECINFO_LIBRARY execinfo)

absl_cc_library(
  NAME
    stacktrace
  HDRS
    "stacktrace.h"
    "internal/stacktrace_aarch64-inl.inc"
    "internal/stacktrace_arm-inl.inc"
    "internal/stacktrace_config.h"
    "internal/stacktrace_emscripten-inl.inc"
    "internal/stacktrace_generic-inl.inc"
    "internal/stacktrace_powerpc-inl.inc"
    "internal/stacktrace_riscv-inl.inc"
    "internal/stacktrace_unimplemented-inl.inc"
    "internal/stacktrace_win32-inl.inc"
    "internal/stacktrace_x86-inl.inc"
  SRCS
    "stacktrace.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    $<$<BOOL:${EXECINFO_LIBRARY}>:${EXECINFO_LIBRARY}>
  DEPS
    turbo::debugging_internal
    turbo::config
    turbo::core_headers
    turbo::raw_logging_internal
  PUBLIC
)

absl_cc_test(
  NAME
    stacktrace_test
  SRCS
    "stacktrace_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::stacktrace
    turbo::core_headers
    GTest::gmock_main
)

absl_cc_library(
  NAME
    symbolize
  HDRS
    "symbolize.h"
    "internal/symbolize.h"
  SRCS
    "symbolize.cc"
    "symbolize_darwin.inc"
    "symbolize_elf.inc"
    "symbolize_emscripten.inc"
    "symbolize_unimplemented.inc"
    "symbolize_win32.inc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
    $<$<BOOL:${MINGW}>:-ldbghelp>
  DEPS
    turbo::debugging_internal
    turbo::demangle_internal
    turbo::base
    turbo::config
    turbo::core_headers
    turbo::dynamic_annotations
    turbo::malloc_internal
    turbo::raw_logging_internal
    turbo::strings
  PUBLIC
)

absl_cc_test(
  NAME
    symbolize_test
  SRCS
    "symbolize_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
    $<$<BOOL:${MSVC}>:-Z7>
  LINKOPTS
    $<$<BOOL:${MSVC}>:-DEBUG>
  DEPS
    turbo::stack_consumption
    turbo::symbolize
    turbo::base
    turbo::config
    turbo::core_headers
    turbo::memory
    turbo::raw_logging_internal
    turbo::strings
    GTest::gmock
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    examine_stack
  HDRS
    "internal/examine_stack.h"
  SRCS
    "internal/examine_stack.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::stacktrace
    turbo::symbolize
    turbo::config
    turbo::core_headers
    turbo::raw_logging_internal
)

absl_cc_library(
  NAME
    failure_signal_handler
  HDRS
    "failure_signal_handler.h"
  SRCS
    "failure_signal_handler.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::examine_stack
    turbo::stacktrace
    turbo::base
    turbo::config
    turbo::core_headers
    turbo::raw_logging_internal
  PUBLIC
)

absl_cc_test(
  NAME
    failure_signal_handler_test
  SRCS
    "failure_signal_handler_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::failure_signal_handler
    turbo::stacktrace
    turbo::symbolize
    turbo::strings
    turbo::raw_logging_internal
    Threads::Threads
    GTest::gmock
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    debugging_internal
  HDRS
    "internal/address_is_readable.h"
    "internal/elf_mem_image.h"
    "internal/vdso_support.h"
  SRCS
    "internal/address_is_readable.cc"
    "internal/elf_mem_image.cc"
    "internal/vdso_support.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::core_headers
    turbo::config
    turbo::dynamic_annotations
    turbo::errno_saver
    turbo::raw_logging_internal
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    demangle_internal
  HDRS
    "internal/demangle.h"
  SRCS
    "internal/demangle.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::core_headers
  PUBLIC
)

absl_cc_test(
  NAME
    demangle_test
  SRCS
    "internal/demangle_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::demangle_internal
    turbo::stack_consumption
    turbo::config
    turbo::core_headers
    turbo::memory
    turbo::raw_logging_internal
    GTest::gmock_main
)

absl_cc_library(
  NAME
    leak_check
  HDRS
    "leak_check.h"
  SRCS
    "leak_check.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::core_headers
  PUBLIC
)

absl_cc_test(
  NAME
    leak_check_test
  SRCS
    "leak_check_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    turbo::leak_check
    turbo::base
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    stack_consumption
  HDRS
    "internal/stack_consumption.h"
  SRCS
    "internal/stack_consumption.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::core_headers
    turbo::raw_logging_internal
  TESTONLY
)

absl_cc_test(
  NAME
    stack_consumption_test
  SRCS
    "internal/stack_consumption_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::stack_consumption
    turbo::core_headers
    turbo::raw_logging_internal
    GTest::gmock_main
)

# component target
absl_cc_library(
  NAME
    debugging
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::stacktrace
    turbo::leak_check
  PUBLIC
)
