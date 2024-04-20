#
# Copyright 2023-2024 The EA Authors.
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

set(CPACK_GENERATOR "STGZ")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME}")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION
"collie is header only c++ 17 library"
)
set(CPACK_PACKAGE_MAINTAINER "Jeff.li")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/gottingen/collie")

cmake_host_system_information(RESULT PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
message(STATUS "${PRETTY_NAME}")
cmake_host_system_information(RESULT DISTRO QUERY DISTRIB_INFO)
foreach(VAR IN LISTS DISTRO)
  message(STATUS "${VAR}=`${${VAR}}`")
endforeach()
set(HOST_SYSTEM_NAME "rh")
if (${PRETTY_NAME} MATCHES "Ubuntu")
  MESSAGE(STATUS "Linux dist: ubuntu")
  set(HOST_SYSTEM_NAME "deb")
elseif (${PRETTY_NAME} MATCHES "centos")
    MESSAGE(STATUS "Linux dist: ubuntu")
    set(HOST_SYSTEM_NAME "rh")
endif ()

set (TAR_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
if (DEFINED CUDA_VERSION)
  set (TAR_FILE_NAME "${TAR_FILE_NAME}-cu${CUDA_VERSION}")
elseif (DEFINED CUDAToolkit_VERSION)
  set (TAR_FILE_NAME "${TAR_FILE_NAME}-cu${CUDAToolkit_VERSION}")
endif ()
set(CPACK_PACKAGE_FILE_NAME "${TAR_FILE_NAME}")

set(CPACK_PACKAGE_DIRECTORY package)
include(CPack)