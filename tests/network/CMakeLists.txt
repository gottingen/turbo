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
        lib_test
        SOURCES
        "test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        thread_pool_test
        SOURCES
        "thread_pool_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        udp_server_test
        SOURCES
        "udp_server_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        event_poller_test
        SOURCES
        "event_poller_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        tcp_client_test
        SOURCES
        "tcp_client_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        tcp_server_test
        SOURCES
        "tcp_server_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        timer_test
        SOURCES
        "timer_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        thread_pool_benchmark_test
        SOURCES
        "thread_pool_benchmark.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        network_resource_pool_test
        SOURCES
        "resource_pool_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        pipe_test
        SOURCES
        "pipe_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        dolaytask_test
        SOURCES
        "dolaytask_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        notice_center_test
        SOURCES
        "notice_center_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        ini_test
        SOURCES
        "ini_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        logger_test
        SOURCES
        "logger_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        create_test
        SOURCES
        "create_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)