#
# Copyright 2023 The Carbin Authors.
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

if (CARBIN_BUILD_TEST)
    enable_testing()
    include(require_gtest)
    include(require_gmock)
endif (CARBIN_BUILD_TEST)

if (CARBIN_BUILD_BENCHMARK)
    include(require_benchmark)
endif ()

set(CARBIN_SYSTEM_DYLINK)
if (APPLE)
    find_library(CoreFoundation CoreFoundation)
    list(APPEND CARBIN_SYSTEM_DYLINK ${CoreFoundation} pthread)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND CARBIN_SYSTEM_DYLINK rt dl pthread)
endif ()

find_package(Threads REQUIRED)
#include(require_turbo)

set(CARBIN_DEPS_LINK
#        ${TURBO_LIB}
        ${CARBIN_SYSTEM_DYLINK}
        Threads::Threads
        )






