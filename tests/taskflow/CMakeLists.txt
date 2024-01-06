#
# Copyright 2023 The titan-search Authors.
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

carbin_cc_test(
        NAME tf_asyncs_test
        SOURCES
        asyncs_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_basics_test
        SOURCES
        basics_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_cancellation_test
        SOURCES
        cancellation_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_compositions_test
        SOURCES
        compositions_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_control_flow_test
        SOURCES
        control_flow_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_data_pipelines_test
        SOURCES
        data_pipelines_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_deferred_pipelines_test
        SOURCES
        deferred_pipelines_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_deferred_scalable_pipelines_test
        SOURCES
        deferred_scalable_pipelines_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_dependent_asyncs_test
        SOURCES
        dependent_asyncs_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_exceptions_test
        SOURCES
        exceptions_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_find_test
        SOURCES
        find_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_for_each_test
        SOURCES
        for_each_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_movable_test
        SOURCES
        movable_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_pipelines_test
        SOURCES
        pipelines_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_priorities_test
        SOURCES
        priorities_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_reduce_test
        SOURCES
        reduce_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_runtimes_test
        SOURCES
        runtimes_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_scalable_pipelines_test
        SOURCES
        scalable_pipelines_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_scan_test
        SOURCES
        scan_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_semaphores_test
        SOURCES
        semaphores_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)


carbin_cc_test(
        NAME tf_sort_test
        SOURCES
        sort_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_subflows_test
        SOURCES
        subflows_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_transform_test
        SOURCES
        transform_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_traversals_test
        SOURCES
        traversals_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

carbin_cc_test(
        NAME tf_work_stealing_test
        SOURCES
        work_stealing_test.cc
        COPTS ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)