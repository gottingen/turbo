#
# Copyright 2023 The titan-search Authors.
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

carbin_cc_test(
        NAME
        fiber_waitable_event_test
        SOURCES
        "fiber_waitable_event_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_wait_event_test
        SOURCES
        "fiber_wait_event_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_test
        SOURCES
        "fiber_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_sched_yield_test
        SOURCES
        "fiber_sched_yield_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_mutex_test
        SOURCES
        "fiber_mutex_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_cond_test
        SOURCES
        "fiber_cond_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)
carbin_cc_test(
        NAME
        fiber_fd_test
        SOURCES
        "fiber_io_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_execution_queue_test
        SOURCES
        "fiber_execution_queue_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_runtime_test
        SOURCES
        "fiber_runtime_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_local_test
        SOURCES
        "fiber_local_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_dispatch_test
        SOURCES
        "fiber_dispatch_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_ping_pong_test
        SOURCES
        "fiber_ping_pong_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fiber_session_test
        SOURCES
        "fiber_session_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)
