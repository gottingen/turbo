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

carbin_cc_library(
        NAMESPACE
        turbo
        NAME
        unicode_test_helper
        SOURCES
        "helper/transcode_test_base.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
)

file(GLOB UNICODE_TEST_SRCS "*.cc")
foreach (UNICODE_UT ${UNICODE_TEST_SRCS})
    get_filename_component(UNICODE_UT_WE ${UNICODE_UT} NAME_WE)
    carbin_cc_test(
            NAME
            ${UNICODE_UT_WE}
            SOURCES
            ${UNICODE_UT}
            COPTS
            ${CARBIN_CXX_OPTIONS}
            DEPS
            turbo::turbo
            turbo::unicode_test_helper
    )
endforeach ()

