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
        memory_test
        SOURCES
        "memory_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        object_pool_test
        SOURCES
        "object_pool_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        resource_pool_test
        SOURCES
        "resource_pool_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

carbin_cc_test(
        NAME
        aligned_allocator_test
        SOURCES
        "aligned_allocator_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)
