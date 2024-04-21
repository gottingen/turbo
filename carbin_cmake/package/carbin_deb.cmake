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

set(CPACK_GENERATOR "STGZ;DEB")
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
set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS
        "/usr/local/cuda-${CPACK_CUDA_VERSION_MAJOR}.${CPACK_CUDA_VERSION_MINOR}/lib64/stubs")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_SOURCE_DIR}/package/deb/preinst;${CMAKE_CURRENT_SOURCE_DIR}/package/deb/prerm;${CMAKE_CURRENT_SOURCE_DIR}/package/deb/postrm;${CMAKE_CURRENT_SOURCE_DIR}/package/deb/postinst")
include(CPack)