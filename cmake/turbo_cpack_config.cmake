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

set(CARBIN_PACKAGING_INSTALL_PREFIX "/opt/EA/inf")
set(CARBIN_PACKAGE_VENDOR "${PROJECT_NAME}")
set(CARBIN_PACKAGE_NAME "${PROJECT_NAME}")

set(CARBIN_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CARBIN_PACKAGE_DESCRIPTION
        "kumo-ann is the ann search component of EA projects."
)
configure_file(${PROJECT_SOURCE_DIR}/cmake/deb/postinst.in ${PROJECT_SOURCE_DIR}/cmake/deb/postinst @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/cmake/rpm/postinst.in ${PROJECT_SOURCE_DIR}/cmake/rpm/postinst @ONLY)

set(CARBIN_PACKAGE_MAINTAINER "Jeff.li")
set(CARBIN_PACKAGE_CONTACT "lijippy@163.com")
set(CARBIN_PACKAGE_HOMEPAGE_URL "https://github.com/gottingen/kumo-ann")
set(CARBIN_PACKAGE_SYSTEM_NAME "unknown")

string(TOLOWER ${CARBIN_PRETTY_NAME} LC_CARBIN_PRETTY_NAME)

if(${LC_CARBIN_PRETTY_NAME} MATCHES "ubuntu")
    set(CARBIN_PACKAGE_SYSTEM_NAME "ubuntu-${CARBIN_DISTRO_VERSION_ID}")
    set(CPACK_GENERATOR "STGZ;DEB")
elseif(${LC_CARBIN_PRETTY_NAME} MATCHES "centos")
    set(CARBIN_PACKAGE_SYSTEM_NAME "centos-${CARBIN_DISTRO_VERSION_ID}")
    set(CPACK_GENERATOR "STGZ;RPM")
elseif(${LC_CARBIN_PRETTY_NAME} MATCHES "darwin")
    set(CARBIN_PACKAGE_SYSTEM_NAME "darwin-${CARBIN_DISTRO_VERSION_ID}")
endif()
if (${CARBIN_PACKAGE_SYSTEM_NAME} MATCHES "unknown")
    set(CARBIN_PACKAGE_SYSTEM_NAME "linux") # default to linux  if not set
endif ()

if (${CARBIN_PACKAGE_SYSTEM_NAME} MATCHES "unknown")
    set(CARBIN_PACKAGE_SYSTEM_NAME "linux") # default to linux  if not set
endif ()
set (TAR_FILE_NAME "${CARBIN_PACKAGE_NAME}-${CARBIN_PACKAGE_VERSION}-${CARBIN_PACKAGE_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")


if (DEFINED CUDA_VERSION)
    set (TAR_FILE_NAME "${TAR_FILE_NAME}-cu${CUDA_VERSION}")
elseif (DEFINED CUDAToolkit_VERSION)
    set (TAR_FILE_NAME "${TAR_FILE_NAME}-cu${CUDAToolkit_VERSION}")
endif ()
set(CARBIN_PACKAGE_FILE_NAME "${TAR_FILE_NAME}")
set(CARBIN_PACKAGE_DIRECTORY package)

carbin_print("on platform: ${LC_CARBIN_PRETTY_NAME}")
carbin_print("package format: ${CPACK_GENERATOR}")
carbin_print("package file name: ${CARBIN_PACKAGE_FILE_NAME}")
#################################################################################
# system configuration
#################################################################################
set(CPACK_PACKAGING_INSTALL_PREFIX ${CARBIN_PACKAGING_INSTALL_PREFIX})
set(CPACK_PACKAGE_VENDOR "${CARBIN_PACKAGE_VENDOR}")
set(CPACK_PACKAGE_NAME "${CARBIN_PACKAGE_NAME}")
set(CPACK_PACKAGE_VERSION "${CARBIN_PACKAGE_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION
        "${CARBIN_PACKAGE_DESCRIPTION}"
)
set(CPACK_PACKAGE_MAINTAINER "${CARBIN_PACKAGE_MAINTAINER}")
set(CPACK_PACKAGE_CONTACT "${CARBIN_PACKAGE_CONTACT}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${CARBIN_PACKAGE_HOMEPAGE_URL}")
set(CPACK_PACKAGE_FILE_NAME "${CARBIN_PACKAGE_FILE_NAME}")
set(CPACK_PACKAGE_DIRECTORY ${CARBIN_PACKAGE_DIRECTORY})

if (${LC_CARBIN_PRETTY_NAME} MATCHES "ubuntu")
    set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS
            "/usr/local/cuda-${CPACK_CUDA_VERSION_MAJOR}.${CPACK_CUDA_VERSION_MINOR}/${CMAKE_INSTALL_LIBDIR}/stubs")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/preinst;${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/prerm;${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/postrm;${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/postinst")
elseif (${LC_CARBIN_PRETTY_NAME} MATCHES "darwin")
    MESSAGE(STATUS "Apple dist: macos build dmg package")
elseif (${LC_CARBIN_PRETTY_NAME} MATCHES "centos")
    set(CPACK_RPM_PACKAGE_DEBUG 1)
    set(CPACK_RPM_RUNTIME_DEBUGINFO_PACKAGE ON)
    set(CPACK_RPM_PACKAGE_RELOCATABLE ON)
    SET(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/rpm/preinst")
    SET(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/rpm/postinst")
    SET(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/rpm/prerm")
    SET(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/rpm/postrm")
endif ()
include(CPack)