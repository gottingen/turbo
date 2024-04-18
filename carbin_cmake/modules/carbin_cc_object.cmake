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


include(CMakeParseArguments)
include(carbin_config_cxx_opts)
include(carbin_install_dirs)
include(carbin_print)
include(carbin_variable)

################################################################################
# Create a Library.
#
# Example usage:
#
# carbin_cc_object(  NAME myLibrary
#                  NAMESPACE myNamespace
#                  SOURCES
#                       myLib.cpp
#                       myLib_functions.cpp
#                  DEFINITIONS
#                     USE_DOUBLE_PRECISION=1
#                  PUBLIC_INCLUDE_PATHS
#                     ${CMAKE_SOURCE_DIR}/mylib/include
#                  PRIVATE_INCLUDE_PATHS
#                     ${CMAKE_SOURCE_DIR}/include
#                  PRIVATE_LINKED_TARGETS
#                     Threads::Threads
#                  PUBLIC_LINKED_TARGETS
#                     Threads::Threads
#                  LINKED_TARGETS
#                     Threads::Threads
# )
#
# The above example creates an alias target, myNamespace::myLibrary which can be
# linked to by other tar gets.
# PUBLIC_DEFINITIONS -  preprocessor defines which are inherated by targets which
#                       link to this library
#
#
# PUBLIC_INCLUDE_PATHS - include paths which are public, therefore inherted by
#                        targest which link to this library.
#
# PRIVATE_INCLUDE_PATHS - private include paths which are only visible by MyLibrary
#
# LINKED_TARGETS        - targets to link to.
################################################################################
function(carbin_cc_object)
    set(options
            EXCLUDE_SYSTEM
            )
    set(args NAME
            NAMESPACE
            )

    set(list_args
            DEPS
            SOURCES
            HEADERS
            INCLUDES
            PINCLUDES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            )

    cmake_parse_arguments(
            PARSE_ARGV 0
            CARBIN_CC_OBJECT
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${CARBIN_CC_OBJECT_NAME}" STREQUAL "")
        get_filename_component(CARBIN_CC_OBJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        string(REPLACE " " "_" CARBIN_CC_OBJECT_NAME ${CARBIN_CC_OBJECT_NAME})
        carbin_print(" Library, NAME argument not provided. Using folder name:  ${CARBIN_CC_OBJECT_NAME}")
    endif ()

    if (NOT CARBIN_CC_OBJECT_NAMESPACE OR "${CARBIN_CC_OBJECT_NAMESPACE}" STREQUAL "")
        set(CARBIN_CC_OBJECT_NAMESPACE ${PROJECT_NAME})
        carbin_print(" Library, NAMESPACE argument not provided. Using target alias:  ${CARBIN_CC_OBJECT_NAME}::${CARBIN_CC_OBJECT_NAME}")
    endif ()

    if ("${CARBIN_CC_OBJECT_SOURCES}" STREQUAL "")
        carbin_error("no source give to the lib ${CARBIN_CC_OBJECT_NAME}, using carbin_cc_object instead")
    endif ()

    carbin_raw("-----------------------------------")
    set(CARBIN_LIB_INFO "${CARBIN_CC_OBJECT_NAMESPACE}::${CARBIN_CC_OBJECT_NAME}  OBJECT ")

    set(${CARBIN_CC_OBJECT_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (CARBIN_CC_OBJECT_EXCLUDE_SYSTEM)
        set(${CARBIN_CC_OBJECT_NAME}_INCLUDE_SYSTEM "")
    endif ()

    carbin_print_label("Create Library" "${CARBIN_LIB_INFO}")
    carbin_raw("-----------------------------------")
    if (VERBOSE_CARBIN_BUILD)
        carbin_print_list_label("Sources" CARBIN_CC_OBJECT_SOURCES)
        carbin_print_list_label("Deps" CARBIN_CC_OBJECT_DEPS)
        carbin_print_list_label("COPTS" CARBIN_CC_OBJECT_COPTS)
        carbin_print_list_label("CXXOPTS" CARBIN_CC_OBJECT_CXXOPTS)
        carbin_print_list_label("CUOPTS" CARBIN_CC_OBJECT_CUOPTS)
        carbin_print_list_label("Defines" CARBIN_CC_OBJECT_DEFINES)
        carbin_print_list_label("Includes" CARBIN_CC_OBJECT_INCLUDES)
        carbin_print_list_label("Private Includes" CARBIN_CC_OBJECT_PINCLUDES)
        carbin_raw("-----------------------------------")
    endif ()
    add_library(${CARBIN_CC_OBJECT_NAME} OBJECT ${CARBIN_CC_OBJECT_SOURCES} ${CARBIN_CC_OBJECT_HEADERS})
    if(CARBIN_CC_OBJECT_DEPS)
        add_dependencies(${CARBIN_CC_OBJECT_NAME} ${CARBIN_CC_OBJECT_DEPS})
    endif ()
    set_property(TARGET ${CARBIN_CC_OBJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
    target_compile_options(${CARBIN_CC_OBJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:C>:${CARBIN_CC_OBJECT_COPTS}>)
    target_compile_options(${CARBIN_CC_OBJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CARBIN_CC_OBJECT_CXXOPTS}>)
    target_compile_options(${CARBIN_CC_OBJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CARBIN_CC_OBJECT_CUOPTS}>)
    target_include_directories(${CARBIN_CC_OBJECT_NAME} ${${CARBIN_CC_OBJECT_NAME}_INCLUDE_SYSTEM}
                PUBLIC
                ${CARBIN_CC_OBJECT_INCLUDES}
                "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>"
                "$<BUILD_INTERFACE:${${PROJECT_NAME}_BINARY_DIR}>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        )
    target_include_directories(${CARBIN_CC_OBJECT_NAME} ${${CARBIN_CC_OBJECT_NAME}_INCLUDE_SYSTEM}
            PRIVATE
            ${CARBIN_CC_OBJECT_PINCLUDES}
    )

    target_compile_definitions(${CARBIN_CC_OBJECT_NAME}
            PUBLIC
            ${CARBIN_CC_OBJECT_DEFINES}
    )

    foreach (arg IN LISTS CARBIN_CC_OBJECT_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed argument: ${arg}")
    endforeach ()

endfunction()

