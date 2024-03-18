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
    #include(require_doctest)
endif (CARBIN_BUILD_TEST)

if (CARBIN_BUILD_BENCHMARK)
    #include(require_benchmark)
endif ()

find_package(Threads REQUIRED)
#include(require_turbo)

############################################################
#
# add you libs to the CARBIN_DEPS_LINK variable eg as turbo
# so you can and system pthread and rt, dl already add to
# CARBIN_SYSTEM_DYLINK, using it for fun.
##########################################################
if (WIN32)
    list(APPEND SUB_DIR_LIST "win32")
    #防止Windows.h包含Winsock.h
    #Prevent Windows.h from including Winsock.h
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

set(ENABLE_OPENSSL ON CACHE BOOL "enable openssl")
set(ASAN_USE_DELETE OFF CACHE BOOL "use delele[] or free when asan enabled")

#查找openssl是否安装
#Find out if openssl is installed
find_package(OpenSSL QUIET)
if (OPENSSL_FOUND AND ENABLE_OPENSSL)
    message(STATUS "找到openssl库:\"${OPENSSL_INCLUDE_DIR}\",ENABLE_OPENSSL宏已打开")
    include_directories(${OPENSSL_INCLUDE_DIR})
    add_definitions(-DENABLE_OPENSSL)
endif ()

#是否使用delete[]替代free，用于解决开启asan后在MacOS上的卡死问题
#use delele[] or free when asan enabled
if(ASAN_USE_DELETE)
    add_definitions(-DASAN_USE_DELETE)
endif()

set(CARBIN_DEPS_LINK
        #${TURBO_LIB}
        ${CARBIN_SYSTEM_DYLINK}
        ${OPENSSL_LIBRARIES}
        dl
        pthread
        )
list(REMOVE_DUPLICATES CARBIN_DEPS_LINK)
carbin_print_list_label("Denpendcies:" CARBIN_DEPS_LINK)