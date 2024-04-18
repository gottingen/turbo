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
# carbin_cc_library(  NAME myLibrary
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
function(carbin_cc_interface)
    set(options
            PUBLIC
            EXCLUDE_SYSTEM
            )
    set(args NAME
            NAMESPACE
            )

    set(list_args
            HEADERS
            COPTS
            CXXOPTS
            CUOPTS
            DEFINES
            )

    cmake_parse_arguments(
            PARSE_ARGV 0
            CARBIN_CC_INTERFACE
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${CARBIN_CC_INTERFACE_NAME}" STREQUAL "")
        get_filename_component(CARBIN_CC_INTERFACE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        string(REPLACE " " "_" CARBIN_CC_INTERFACE_NAME ${CARBIN_CC_INTERFACE_NAME})
        carbin_print(" Library, NAME argument not provided. Using folder name:  ${CARBIN_CC_INTERFACE_NAME}")
    endif ()

    if ("${CARBIN_CC_INTERFACE_NAMESPACE}" STREQUAL "")
        set(CARBIN_CC_INTERFACE_NAMESPACE ${PROJECT_NAME})
        message(" Library, NAMESPACE argument not provided. Using target alias:  ${CARBIN_CC_INTERFACE_NAME}::${CARBIN_CC_INTERFACE_NAME}")
    endif ()

    if ("${CARBIN_CC_INTERFACE_HEADERS}" STREQUAL "")
        carbin_error("no source give to the interface ${CARBIN_CC_INTERFACE_NAME}")
    endif ()

    set(${CARBIN_CC_INTERFACE_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (CARBIN_CC_INTERFACE_EXCLUDE_SYSTEM)
        set(${CARBIN_CC_INTERFACE_NAME}_INCLUDE_SYSTEM "")
    endif ()

    carbin_raw("-----------------------------------")
    if(CARBIN_CC_INTERFACE_PUBLIC)
        set(CARBIN_CC_INTERFACE_INFO "${CARBIN_CC_INTERFACE_NAMESPACE}::${CARBIN_CC_INTERFACE_NAME}  INTERFACE PUBLIC")
    else()
        set(CARBIN_CC_INTERFACE_INFO "${CARBIN_CC_INTERFACE_NAMESPACE}::${CARBIN_CC_INTERFACE_NAME}  INTERFACE INTERNAL")
    endif ()
    carbin_print_label("Create Library" "${CARBIN_CC_INTERFACE_INFO}")
    carbin_raw("-----------------------------------")
    if (VERBOSE_CARBIN_BUILD)
        carbin_print_list_label("Headers" CARBIN_CC_INTERFACE_HEADERS)
        carbin_raw("-----------------------------------")
    endif ()

    add_library(${CARBIN_CC_INTERFACE_NAME} INTERFACE)
    add_library(${CARBIN_CC_INTERFACE_NAMESPACE}::${CARBIN_CC_INTERFACE_NAME} ALIAS ${CARBIN_CC_INTERFACE_NAME})

    target_compile_options(${CARBIN_CC_INTERFACE_NAME} INTERFACE $<$<COMPILE_LANGUAGE:C>:${CARBIN_CC_INTERFACE_COPTS}>)
    target_compile_options(${CARBIN_CC_INTERFACE_NAME} INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${CARBIN_CC_INTERFACE_CXXOPTS}>)
    target_compile_options(${CARBIN_CC_INTERFACE_NAME} INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${CARBIN_CC_INTERFACE_CUOPTS}>)

    target_include_directories(${CARBIN_CC_INTERFACE_NAME} ${${CARBIN_CC_INTERFACE_NAME}_INCLUDE_SYSTEM}
            INTERFACE
            "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${${PROJECT_NAME}_BINARY_DIR}>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
            )

    target_compile_definitions(${CARBIN_CC_INTERFACE_NAME} INTERFACE ${CARBIN_CC_INTERFACE_DEFINES})


    if (CARBIN_CC_INTERFACE_PUBLIC)
        install(TARGETS ${CARBIN_CC_INTERFACE_NAME}
                EXPORT ${PROJECT_NAME}Targets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif ()

    foreach (arg IN LISTS CARBIN_CC_INTERFACE_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed argument: ${arg}")
    endforeach ()

endfunction()

