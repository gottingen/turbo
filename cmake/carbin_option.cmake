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

option(BUILD_STATIC_LIBRARY "carbin set build static library or not" ON)

option(BUILD_SHARED_LIBRARY "carbin set build shared library or not" OFF)

option(VERBOSE_CARBIN_BUILD "print carbin detail information" OFF)

option(VERBOSE_CMAKE_BUILD " " OFF)

option(CONDA_ENV_ENABLE " " OFF)

option(CARBIN_USE_CXX11_ABI " " ON)

option(CARBIN_BUILD_TEST "" ON)

option(CARBIN_BUILD_BENCHMARK "" OFF)

option(CARBIN_BUILD_EXAMPLES "" ON)

option(CARBIN_DEPS_ENABLE " " ON)

option(CARBIN_USE_SYSTEM_INCLUDES "" OFF)
if (VERBOSE_CMAKE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()

if (CARBIN_USE_CXX11_ABI)
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
elseif ()
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif ()

if (CONDA_ENV_ENABLE)
    list(APPEND CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX})
    include_directories($ENV{CONDA_PREFIX}/include)
    link_directories($ENV{CONDA_PREFIX}/lib)
endif ()

if (CARBIN_DEPS_ENABLE)
    list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/carbin)
endif ()

if (CARBIN_USE_SYSTEM_INCLUDES)
    set(CARBIN_INTERNAL_INCLUDE_WARNING_GUARD SYSTEM)
else ()
    set(CARBIN_INTERNAL_INCLUDE_WARNING_GUARD "")
endif ()

#################################
#user defines
######################################

option(ENABLE_CUDA "" OFF)