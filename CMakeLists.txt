#
# Copyright (C) 2024 EA group inc.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
####################################################################################
# generate by carbin package manager tools for titan-search
# https://github.com/gottingen/carbin
####################################################################################
cmake_minimum_required(VERSION 3.24)

project(turbo CXX C)

option(${PROJECT_NAME}_IS_TOP_PROJECT "turbo project is the top level project or not" ON)

get_property(not_top DIRECTORY PROPERTY PARENT_DIRECTORY)
if (not_top)
    set(${PROJECT_NAME}_IS_TOP_PROJECT OFF)
endif ()
message(STATUS ${PROJECT_NAME}_IS_TOP_PROJECT)

set(PROJECT_DESCRIPTION "turbo is collection of high performance c++ containers and algorithms")
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 5)
set(PROJECT_VERSION_PATCH 6)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(${PROJECT_NAME}_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(${PROJECT_NAME}_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(${PROJECT_NAME}_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(${PROJECT_NAME}_VERSION ${PROJECT_VERSION})


set(CARBIN_VERSION 0.5.0)
set(CARBIN_CMAKE_VERSION 0.5.0)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/carbin_cmake)
##################################################################
# add this for ${PROJECT_NAME} 's cmake include
##################################################################
list(APPEND CMAKE_MODULE_PATH ${${PROJECT_NAME}_SOURCE_DIR}/cmake)
#################################################################
#
# modify the CARBIN_DEPS_PREFIX to you prefix if needed.
# for example when you install you deps to "/opt/handsome/guy
# modify as set(CARBIN_DEPS_PREFIX /opt/handsome/guy)
# this should be PROJECT_SOURCE_DIR, only install libraries
# deps to top level project
#################################################################
set(CARBIN_DEPS_PREFIX ${PROJECT_SOURCE_DIR}/carbin)
#######################################################################
# warning
# in carbin_cmake directory, caution to modify files in that dir
######################################################################
include(carbin_module)

##################################################################
#
# your cmake directory ${PROJECT_SOURCE_DIR}/cmake, do things here
#################################################################
include(turbo_deps)
include(turbo_cxx_config)
include(turbo_test)

configure_file(${PROJECT_SOURCE_DIR}/conda/meta.yaml.in ${PROJECT_SOURCE_DIR}/conda/meta.yaml @ONLY)

add_subdirectory(turbo)
####################################################################
# belows are auto, edit it be cation
####################################################################
if (CARBIN_BUILD_TEST)
    add_subdirectory(tests)
endif ()

if (CARBIN_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif ()

if (CARBIN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

##############################################
# header installing
#############################################
install(DIRECTORY ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.inc"
        PATTERN "*.h"
        PATTERN "*.hpp"
        )

# Layout. This works for all platforms:
#   * <prefix>/@CMAKE_INSTALL_LIBDIR@/cmake/<PROJECT-NAME>
#   * <prefix>/@CMAKE_INSTALL_LIBDIR@*/
#   * <prefix>/include/
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

# Include module with fuction 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Use:
#   * PROJECT_VERSION
write_basic_package_version_file(
        "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * TARGETS_EXPORT_NAME
#   * PROJECT_NAME
configure_package_config_file(
        "cmake/turbo_config.cmake.in"
        "${project_config}"
        INSTALL_DESTINATION "${config_install_dir}"
)

#Targets:
#  * <prefix>/${CMAKE_INSTALL_LIBDIR}/libbar.a
#  * <prefix>/${CMAKE_INSTALL_LIBDIR}/libbaz.a
#  * header location after install: <prefix>/include/foo/Bar.hpp
#  * headers can be included by C++ code `#include <foo/Bar.hpp>`


# Config
#   * <prefix>/${CMAKE_INSTALL_LIBDIR}/cmake/Foo/FooConfig.cmake
#   * <prefix>/${CMAKE_INSTALL_LIBDIR}/cmake/Foo/FooConfigVersion.cmake
install(
        FILES
        "${project_config}" "${version_config}"
        DESTINATION
        "${config_install_dir}"
)

# Config
#   * <prefix>/${CMAKE_INSTALL_LIBDIR}/cmake/Foo/FooTargets.cmake

install(
        EXPORT
        "${TARGETS_EXPORT_NAME}"
        NAMESPACE
        "${namespace}"
        DESTINATION
        "${config_install_dir}"
)
configure_file(${PROJECT_SOURCE_DIR}/carbin_cmake/package/pkg_dump_template.pc.in
        ${PROJECT_BINARY_DIR}/package/${PROJECT_NAME}.pc)

install(FILES ${PROJECT_BINARY_DIR}/package/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
include(turbo_cpack_config)