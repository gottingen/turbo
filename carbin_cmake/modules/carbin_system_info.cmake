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

cmake_host_system_information(RESULT PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
message(STATUS "${PRETTY_NAME}")
cmake_host_system_information(RESULT DISTRO QUERY DISTRIB_INFO)
foreach(VAR IN LISTS DISTRO)
    message(STATUS "${VAR}=`${${VAR}}`")
endforeach()

set(CARBIN_PACKAGE_SYSTEM_NAME "unknown")
if (${PRETTY_NAME} MATCHES "Ubuntu")
    set(CARBIN_PACKAGE_SYSTEM_NAME "ubuntu-${DISTRO_VERSION_ID}")
elseif (${PRETTY_NAME} MATCHES "darwin")
    MESSAGE(STATUS "Linux dist: macos")
    set(HOST_SYSTEM_NAME "dmg")
elseif (${PRETTY_NAME} MATCHES "centos")
    MESSAGE(STATUS "Linux dist: ubuntu")
    set(HOST_SYSTEM_NAME "rh")
endif ()
