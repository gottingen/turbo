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
find_package(Threads REQUIRED)
carbin_cc_test(
        NAME
        latch_test
        MODULE con
        SOURCES
        "latch_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        Threads::Threads
)

carbin_cc_test(
        NAME
        barrier_test
        MODULE con
        SOURCES
        "barrier_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        Threads::Threads
)

carbin_cc_test(
        NAME
        doubly_buffered_data_test
        MODULE con
        SOURCES
        "doubly_buffered_data_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        Threads::Threads
)

carbin_cc_test(
        NAME
        thread_local_test
        MODULE con
        SOURCES
        "thread_local_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        Threads::Threads
)

carbin_cc_test(
        NAME
        work_stealing_queue_test
        MODULE con
        SOURCES
        "work_stealing_queue_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        Threads::Threads
)

carbin_cc_test(
        NAME
        spinlock_test
        MODULE con
        SOURCES
        "spinlock_common_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        Threads::Threads
)

carbin_cc_test(
        NAME
        spin_lock_wait_test
        MODULE con
        SOURCES
        "spin_lock_wait_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        Threads::Threads
)
