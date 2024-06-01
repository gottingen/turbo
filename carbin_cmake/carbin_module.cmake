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
if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/carbin_cmake/modules)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/carbin_cmake/package)
################################################################################################
# options
################################################################################################
option(BUILD_STATIC_LIBRARY "carbin set build static library or not" ON)

option(BUILD_SHARED_LIBRARY "carbin set build shared library or not" OFF)

option(VERBOSE_CARBIN_BUILD "print carbin detail information" OFF)

option(VERBOSE_CMAKE_BUILD "verbose cmake make debug" OFF)

option(CONDA_ENV_ENABLE "enable conda auto env" OFF)

option(CARBIN_USE_CXX11_ABI "use cxx11 abi or not" ON)

option(CARBIN_BUILD_TEST "enable project test or not" ON)

option(CARBIN_BUILD_BENCHMARK "enable project benchmark or not" OFF)

option(CARBIN_BUILD_EXAMPLES "enable project examples or not" OFF)

option(CARBIN_DEPS_ENABLE " " ON)

option(CARBIN_ENABLE_ARCH "" ON)

option(CARBIN_ENABLE_CUDA "" OFF)

option(CARBIN_STATUS_PRINT "carbin print or not, default on" ON)

option(CARBIN_INSTALL_LIB "avoid centos install to lib64" OFF)
option(WITH_DEBUG_SYMBOLS "With debug symbols" ON)

################################################################################################
# color
################################################################################################
if (NOT WIN32)
    string(ASCII 27 Esc)
    set(carbin_colour_reset "${Esc}[m")
    set(carbin_colour_bold "${Esc}[1m")
    set(carbin_red "${Esc}[31m")
    set(carbin_green "${Esc}[32m")
    set(carbin_yellow "${Esc}[33m")
    set(carbin_blue "${Esc}[34m")
    set(carbin_agenta "${Esc}[35m")
    set(carbin_cyan "${Esc}[36m")
    set(carbin_white "${Esc}[37m")
    set(carbin_bold_red "${Esc}[1;31m")
    set(carbin_bold_green "${Esc}[1;32m")
    set(carbin_bold_yellow "${Esc}[1;33m")
    set(carbin_bold_blue "${Esc}[1;34m")
    set(carbin_bold_magenta "${Esc}[1;35m")
    set(carbin_bold_cyan "${Esc}[1;36m")
    set(carbin_bold_white "${Esc}[1;37m")
endif ()

################################################################################################
# print
################################################################################################
function(carbin_debug)
    if (CARBIN_STATUS_DEBUG)
        string(TIMESTAMP timestamp)
        if (CARBIN_CACHE_RUN)
            set(type "DEBUG (CACHE RUN)")
        else ()
            set(type "DEBUG")
        endif ()
        message(STATUS "${carbin_blue}[carbin *** ${type} *** ${timestamp}] ${ARGV}${carbin_colour_reset}")
    endif ()
endfunction(carbin_debug)

function(carbin_print)
    if (CARBIN_STATUS_PRINT OR CARBIN_STATUS_DEBUG)
        if (CARBIN_CACHE_RUN)
            carbin_debug("${ARGV}")
        else ()
            message(STATUS "${carbin_green}[carbin] ${ARGV}${carbin_colour_reset}")
        endif ()
    endif ()
endfunction(carbin_print)

function(carbin_error)
    message("")
    foreach (print_message ${ARGV})
        message(SEND_ERROR "${carbin_bold_red}[carbin ** INTERNAL **] ${print_message}${carbin_colour_reset}")
    endforeach ()
    message(FATAL_ERROR "${carbin_bold_red}[carbin ** INTERNAL **] [Directory:${CMAKE_CURRENT_LIST_DIR}]${carbin_colour_reset}")
    message("")
endfunction(carbin_error)

function(carbin_warn)
    message("")
    foreach (print_message ${ARGV})
        message(WARNING "${carbin_red}[carbin WARNING] ${print_message}${carbin_colour_reset}")
    endforeach ()
    message(WARNING "${carbin_red}[carbin WARNING] [Directory:${CMAKE_CURRENT_LIST_DIR}]${carbin_colour_reset}")
    message("")
endfunction(carbin_warn)

set(CARBIN_ALIGN_LENGTH 30)
MACRO(carbin_print_label Label Value)
    string(LENGTH ${Label} lLength)
    math(EXPR paddingLeng ${CARBIN_ALIGN_LENGTH}-${lLength})
    string(REPEAT " " ${paddingLeng} PADDING)
    message("${carbin_yellow}${Label}${carbin_colour_reset}:${PADDING}${carbin_cyan}${Value}${carbin_colour_reset}")
ENDMACRO()

MACRO(carbin_raw Value)
    message("${Value}")
ENDMACRO()

MACRO(carbin_directory_list result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH (child ${children})
        IF (IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF ()
    ENDFOREACH ()
    SET(${result} ${dirlist})
ENDMACRO()

MACRO(carbin_print_list result)
    foreach (arg IN LISTS ${result})
        message(" - ${carbin_cyan}${arg}${carbin_colour_reset}")
    endforeach ()
ENDMACRO()


MACRO(carbin_print_list_label Label ListVar)
    message("${carbin_yellow}${Label}${carbin_colour_reset}:")
    carbin_print_list(${ListVar})
ENDMACRO()

################################################################################################
# platform info
################################################################################################
cmake_host_system_information(RESULT CARBIN_PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)

carbin_print("${CARBIN_PRETTY_NAME}")

cmake_host_system_information(RESULT CARBIN_DISTRO QUERY DISTRIB_INFO)
carbin_print_list_label("CARBIN_DISTRO:" CARBIN_DISTRO)
foreach (dis IN LISTS CARBIN_DISTRO)
    carbin_print("${dis} = `${${dis}}`")
endforeach ()


################################################################################################
# install dir
################################################################################################
include(GNUInstallDirs)

if (${PROJECT_NAME}_VERSION)
    set(CARBIN_SUBDIR "${PROJECT_NAME}_${PROJECT_VERSION}")
    set(CARBIN_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}/${CARBIN_SUBDIR}")
    set(CARBIN_INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${CARBIN_SUBDIR}")
    set(CARBIN_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/{CARBIN_SUBDIR}")
    set(CARBIN_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${CARBIN_SUBDIR}")
else ()
    set(CARBIN_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
    set(CARBIN_INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
    set(CARBIN_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
    set(CARBIN_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
endif ()

################################################################################################
# carbin_cc_library
################################################################################################

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
#                  DEFINES
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
# PUBLIC_DEFINES -  preprocessor defines which are inherated by targets which
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
function(carbin_cc_library)
    set(options
            PUBLIC
            EXCLUDE_SYSTEM
    )
    set(args NAME
            NAMESPACE
    )

    set(list_args
            DEPS
            SOURCES
            OBJECTS
            HEADERS
            INCLUDES
            PINCLUDES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            LINKS
            PLINKS
            WLINKS
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            CARBIN_CC_LIB
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${CARBIN_CC_LIB_NAME}" STREQUAL "")
        get_filename_component(CARBIN_CC_LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        string(REPLACE " " "_" CARBIN_CC_LIB_NAME ${CARBIN_CC_LIB_NAME})
        carbin_print(" Library, NAME argument not provided. Using folder name:  ${CARBIN_CC_LIB_NAME}")
    endif ()

    if ("${CARBIN_CC_LIB_NAMESPACE}" STREQUAL "")
        set(CARBIN_CC_LIB_NAMESPACE ${PROJECT_NAME})
        carbin_print(" Library, NAMESPACE argument not provided. Using target alias:  ${CARBIN_CC_LIB_NAME}::${CARBIN_CC_LIB_NAME}")
    endif ()

    carbin_raw("-----------------------------------")
    if (CARBIN_CC_LIB_PUBLIC)
        set(CARBIN_LIB_INFO "${CARBIN_CC_LIB_NAMESPACE}::${CARBIN_CC_LIB_NAME}  SHARED&STATIC PUBLIC")
    else ()
        set(CARBIN_LIB_INFO "${CARBIN_CC_LIB_NAMESPACE}::${CARBIN_CC_LIB_NAME}  SHARED&STATIC INTERNAL")
    endif ()

    set(${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (CARBIN_CC_LIB_EXCLUDE_SYSTEM)
        set(${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM "")
    endif ()

    carbin_print_label("Create Library" "${CARBIN_LIB_INFO}")
    carbin_raw("-----------------------------------")
    if (VERBOSE_CARBIN_BUILD)
        carbin_print_list_label("Sources" CARBIN_CC_LIB_SOURCES)
        carbin_print_list_label("Objects" CARBIN_CC_LIB_OBJECTS)
        carbin_print_list_label("Deps" CARBIN_CC_LIB_DEPS)
        carbin_print_list_label("COPTS" CARBIN_CC_LIB_COPTS)
        carbin_print_list_label("CXXOPTS" CARBIN_CC_LIB_CXXOPTS)
        carbin_print_list_label("CUOPTS" CARBIN_CC_LIB_CUOPTS)
        carbin_print_list_label("Defines" CARBIN_CC_LIB_DEFINES)
        carbin_print_list_label("Includes" CARBIN_CC_LIB_INCLUDES)
        carbin_print_list_label("Private Includes" CARBIN_CC_LIB_PINCLUDES)
        carbin_print_list_label("Links" CARBIN_CC_LIB_LINKS)
        carbin_print_list_label("Private Links" CARBIN_CC_LIB_PLINKS)
        carbin_raw("-----------------------------------")
    endif ()
    set(CARBIN_CC_LIB_OBJECTS_FLATTEN)
    if (CARBIN_CC_LIB_OBJECTS)
        foreach (obj IN LISTS CARBIN_CC_LIB_OBJECTS)
            list(APPEND CARBIN_CC_LIB_OBJECTS_FLATTEN $<TARGET_OBJECTS:${obj}>)
        endforeach ()
    endif ()
    if (CARBIN_CC_LIB_SOURCES)
        add_library(${CARBIN_CC_LIB_NAME}_OBJECT OBJECT ${CARBIN_CC_LIB_SOURCES} ${CARBIN_CC_LIB_HEADERS})
        list(APPEND CARBIN_CC_LIB_OBJECTS_FLATTEN $<TARGET_OBJECTS:${CARBIN_CC_LIB_NAME}_OBJECT>)
        if (CARBIN_CC_LIB_DEPS)
            add_dependencies(${CARBIN_CC_LIB_NAME}_OBJECT ${CARBIN_CC_LIB_DEPS})
        endif ()
        set_property(TARGET ${CARBIN_CC_LIB_NAME}_OBJECT PROPERTY POSITION_INDEPENDENT_CODE 1)
        target_compile_options(${CARBIN_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:C>:${CARBIN_CC_LIB_COPTS}>)
        target_compile_options(${CARBIN_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CARBIN_CC_LIB_CXXOPTS}>)
        target_compile_options(${CARBIN_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CARBIN_CC_LIB_CUOPTS}>)
        target_include_directories(${CARBIN_CC_LIB_NAME}_OBJECT ${${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM}
                PUBLIC
                ${CARBIN_CC_LIB_INCLUDES}
                "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>"
                "$<BUILD_INTERFACE:${${PROJECT_NAME}_BINARY_DIR}>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        )
        target_include_directories(${CARBIN_CC_LIB_NAME}_OBJECT ${${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM}
                PRIVATE
                ${CARBIN_CC_LIB_PINCLUDES}
        )

        target_compile_definitions(${CARBIN_CC_LIB_NAME}_OBJECT
                PUBLIC
                ${CARBIN_CC_LIB_DEFINES}
        )
    endif ()

    list(LENGTH CARBIN_CC_LIB_OBJECTS_FLATTEN obj_len)
    if (obj_len EQUAL -1)
        carbin_error("no source or object give to the library ${CARBIN_CC_LIB_NAME}")
    endif ()
    add_library(${CARBIN_CC_LIB_NAME}_static STATIC ${CARBIN_CC_LIB_OBJECTS_FLATTEN})
    if (${CARBIN_CC_LIB_NAME}_OBJECT)
        add_dependencies(${CARBIN_CC_LIB_NAME}_static ${CARBIN_CC_LIB_NAME}_OBJECT)
    endif ()
    if (CARBIN_CC_LIB_DEPS)
        add_dependencies(${CARBIN_CC_LIB_NAME}_static ${CARBIN_CC_LIB_DEPS})
    endif ()
    target_link_libraries(${CARBIN_CC_LIB_NAME}_static PRIVATE ${CARBIN_CC_LIB_PLINKS})
    target_link_libraries(${CARBIN_CC_LIB_NAME}_static PUBLIC ${CARBIN_CC_LIB_LINKS})
    target_link_libraries(${CARBIN_CC_LIB_NAME}_static PRIVATE ${CARBIN_CC_LIB_WLINKS})
    set_target_properties(${CARBIN_CC_LIB_NAME}_static PROPERTIES
            OUTPUT_NAME ${CARBIN_CC_LIB_NAME})
    add_library(${CARBIN_CC_LIB_NAMESPACE}::${CARBIN_CC_LIB_NAME}_static ALIAS ${CARBIN_CC_LIB_NAME}_static)

    add_library(${CARBIN_CC_LIB_NAME}_shared SHARED ${CARBIN_CC_LIB_OBJECTS_FLATTEN})
    if (${CARBIN_CC_LIB_NAME}_OBJECT)
        add_dependencies(${CARBIN_CC_LIB_NAME}_shared ${CARBIN_CC_LIB_NAME}_OBJECT)
    endif ()
    if (CARBIN_CC_LIB_DEPS)
        add_dependencies(${CARBIN_CC_LIB_NAME}_shared ${CARBIN_CC_LIB_DEPS})
    endif ()
    target_link_libraries(${CARBIN_CC_LIB_NAME}_shared PRIVATE ${CARBIN_CC_LIB_PLINKS})
    target_link_libraries(${CARBIN_CC_LIB_NAME}_shared PUBLIC ${CARBIN_CC_LIB_LINKS})
    foreach (link ${CARBIN_CC_LIB_WLINKS})
        target_link_libraries(${CARBIN_CC_LIB_NAME}_shared PRIVATE $<LINK_LIBRARY:WHOLE_ARCHIVE,${link}>)
    endforeach ()
    set_target_properties(${CARBIN_CC_LIB_NAME}_shared PROPERTIES
            OUTPUT_NAME ${CARBIN_CC_LIB_NAME}
            VERSION ${${PROJECT_NAME}_VERSION}
            SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR})
    add_library(${CARBIN_CC_LIB_NAMESPACE}::${CARBIN_CC_LIB_NAME} ALIAS ${CARBIN_CC_LIB_NAME}_shared)

    if (CARBIN_CC_LIB_PUBLIC)
        install(TARGETS ${CARBIN_CC_LIB_NAME}_shared ${CARBIN_CC_LIB_NAME}_static
                EXPORT ${PROJECT_NAME}Targets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif ()

    foreach (arg IN LISTS CARBIN_CC_LIB_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed argument: ${arg}")
    endforeach ()

endfunction()

function(carbin_cc_test_library)
    set(options
            SHARED
            EXCLUDE_SYSTEM
    )
    set(args NAME
            NAMESPACE
    )

    set(list_args
            DEPS
            SOURCES
            OBJECTS
            HEADERS
            INCLUDES
            PINCLUDES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            LINKS
            PLINKS
            WLINKS
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            CARBIN_CC_LIB
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${CARBIN_CC_LIB_NAME}" STREQUAL "")
        get_filename_component(CARBIN_CC_LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        string(REPLACE " " "_" CARBIN_CC_LIB_NAME ${CARBIN_CC_LIB_NAME})
        carbin_print(" Library, NAME argument not provided. Using folder name:  ${CARBIN_CC_LIB_NAME}")
    endif ()

    if ("${CARBIN_CC_LIB_NAMESPACE}" STREQUAL "")
        set(CARBIN_CC_LIB_NAMESPACE ${PROJECT_NAME})
        carbin_print(" Library, NAMESPACE argument not provided. Using target alias:  ${CARBIN_CC_LIB_NAME}::${CARBIN_CC_LIB_NAME}")
    endif ()

    carbin_raw("-----------------------------------")
    if (CARBIN_CC_LIB_SHARED)
        set(CARBIN_LIB_INFO "${CARBIN_CC_LIB_NAMESPACE}::${CARBIN_CC_LIB_NAME}  SHARED&STATIC INTERNAL")
    else ()
        set(CARBIN_LIB_INFO "${CARBIN_CC_LIB_NAMESPACE}::${CARBIN_CC_LIB_NAME}  STATIC INTERNAL")
    endif ()

    set(${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (CARBIN_CC_LIB_EXCLUDE_SYSTEM)
        set(${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM "")
    endif ()

    carbin_print_label("Create Library" "${CARBIN_LIB_INFO}")
    carbin_raw("-----------------------------------")
    if (VERBOSE_CARBIN_BUILD)
        carbin_print_list_label("Sources" CARBIN_CC_LIB_SOURCES)
        carbin_print_list_label("Objects" CARBIN_CC_LIB_OBJECTS)
        carbin_print_list_label("Deps" CARBIN_CC_LIB_DEPS)
        carbin_print_list_label("COPTS" CARBIN_CC_LIB_COPTS)
        carbin_print_list_label("CXXOPTS" CARBIN_CC_LIB_CXXOPTS)
        carbin_print_list_label("CUOPTS" CARBIN_CC_LIB_CUOPTS)
        carbin_print_list_label("Defines" CARBIN_CC_LIB_DEFINES)
        carbin_print_list_label("Includes" CARBIN_CC_LIB_INCLUDES)
        carbin_print_list_label("Private Includes" CARBIN_CC_LIB_PINCLUDES)
        carbin_print_list_label("Links" CARBIN_CC_LIB_LINKS)
        carbin_print_list_label("Private Links" CARBIN_CC_LIB_PLINKS)
        carbin_raw("-----------------------------------")
    endif ()
    set(CARBIN_CC_LIB_OBJECTS_FLATTEN)
    if (CARBIN_CC_LIB_OBJECTS)
        foreach (obj IN LISTS CARBIN_CC_LIB_OBJECTS)
            list(APPEND CARBIN_CC_LIB_OBJECTS_FLATTEN $<TARGET_OBJECTS:${obj}>)
        endforeach ()
    endif ()
    if (CARBIN_CC_LIB_SOURCES)
        add_library(${CARBIN_CC_LIB_NAME}_OBJECT OBJECT ${CARBIN_CC_LIB_SOURCES} ${CARBIN_CC_LIB_HEADERS})
        list(APPEND CARBIN_CC_LIB_OBJECTS_FLATTEN $<TARGET_OBJECTS:${CARBIN_CC_LIB_NAME}_OBJECT>)
        if (CARBIN_CC_LIB_DEPS)
            add_dependencies(${CARBIN_CC_LIB_NAME}_OBJECT ${CARBIN_CC_LIB_DEPS})
        endif ()
        set_property(TARGET ${CARBIN_CC_LIB_NAME}_OBJECT PROPERTY POSITION_INDEPENDENT_CODE 1)
        target_compile_options(${CARBIN_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:C>:${CARBIN_CC_LIB_COPTS}>)
        target_compile_options(${CARBIN_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CARBIN_CC_LIB_CXXOPTS}>)
        target_compile_options(${CARBIN_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CARBIN_CC_LIB_CUOPTS}>)
        target_include_directories(${CARBIN_CC_LIB_NAME}_OBJECT ${${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM}
                PUBLIC
                ${CARBIN_CC_LIB_INCLUDES}
                "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>"
                "$<BUILD_INTERFACE:${${PROJECT_NAME}_BINARY_DIR}>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        )
        target_include_directories(${CARBIN_CC_LIB_NAME}_OBJECT ${${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM}
                PRIVATE
                ${CARBIN_CC_LIB_PINCLUDES}
        )

        target_compile_definitions(${CARBIN_CC_LIB_NAME}_OBJECT
                PUBLIC
                ${CARBIN_CC_LIB_DEFINES}
        )
    endif ()

    list(LENGTH CARBIN_CC_LIB_OBJECTS_FLATTEN obj_len)
    if (obj_len EQUAL -1)
        carbin_error("no source or object give to the library ${CARBIN_CC_LIB_NAME}")
    endif ()
    add_library(${CARBIN_CC_LIB_NAME}_static STATIC ${CARBIN_CC_LIB_OBJECTS_FLATTEN})
    if (${CARBIN_CC_LIB_NAME}_OBJECT)
        add_dependencies(${CARBIN_CC_LIB_NAME}_static ${CARBIN_CC_LIB_NAME}_OBJECT)
    endif ()
    if (CARBIN_CC_LIB_DEPS)
        add_dependencies(${CARBIN_CC_LIB_NAME}_static ${CARBIN_CC_LIB_DEPS})
    endif ()
    target_link_libraries(${CARBIN_CC_LIB_NAME}_static PRIVATE ${CARBIN_CC_LIB_PLINKS})
    target_link_libraries(${CARBIN_CC_LIB_NAME}_static PUBLIC ${CARBIN_CC_LIB_LINKS})
    target_link_libraries(${CARBIN_CC_LIB_NAME}_static PRIVATE ${CARBIN_CC_LIB_WLINKS})
    set_target_properties(${CARBIN_CC_LIB_NAME}_static PROPERTIES
            OUTPUT_NAME ${CARBIN_CC_LIB_NAME})
    add_library(${CARBIN_CC_LIB_NAMESPACE}::${CARBIN_CC_LIB_NAME}_static ALIAS ${CARBIN_CC_LIB_NAME}_static)
    if (CARBIN_CC_LIB_SHARED)
        add_library(${CARBIN_CC_LIB_NAME}_shared SHARED ${CARBIN_CC_LIB_OBJECTS_FLATTEN})
        if (${CARBIN_CC_LIB_NAME}_OBJECT)
            add_dependencies(${CARBIN_CC_LIB_NAME}_shared ${CARBIN_CC_LIB_NAME}_OBJECT)
        endif ()
        if (CARBIN_CC_LIB_DEPS)
            add_dependencies(${CARBIN_CC_LIB_NAME}_shared ${CARBIN_CC_LIB_DEPS})
        endif ()
        target_link_libraries(${CARBIN_CC_LIB_NAME}_shared PRIVATE ${CARBIN_CC_LIB_PLINKS})
        target_link_libraries(${CARBIN_CC_LIB_NAME}_shared PUBLIC ${CARBIN_CC_LIB_LINKS})
        foreach (link ${CARBIN_CC_LIB_WLINKS})
            target_link_libraries(${CARBIN_CC_LIB_NAME}_shared PRIVATE $<LINK_LIBRARY:WHOLE_ARCHIVE,${link}>)
        endforeach ()
        set_target_properties(${CARBIN_CC_LIB_NAME}_shared PROPERTIES
                OUTPUT_NAME ${CARBIN_CC_LIB_NAME}
                VERSION ${${PROJECT_NAME}_VERSION}
                SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR})
        add_library(${CARBIN_CC_LIB_NAMESPACE}::${CARBIN_CC_LIB_NAME} ALIAS ${CARBIN_CC_LIB_NAME}_shared)
    endif ()

    foreach (arg IN LISTS CARBIN_CC_LIB_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed argument: ${arg}")
    endforeach ()

endfunction()

################################################################################################
# carbin_cc_interface
################################################################################################
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
#                  DEFINES
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
# PUBLIC_DEFINES -  preprocessor defines which are inherated by targets which
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
    if (CARBIN_CC_INTERFACE_PUBLIC)
        set(CARBIN_CC_INTERFACE_INFO "${CARBIN_CC_INTERFACE_NAMESPACE}::${CARBIN_CC_INTERFACE_NAME}  INTERFACE PUBLIC")
    else ()
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

################################################################################################
# carbin_cc_object
################################################################################################
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
#                  DEFINES
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
# PUBLIC_DEFINES -  preprocessor defines which are inherated by targets which
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
        carbin_error("no source give to the library ${CARBIN_CC_OBJECT_NAME}, using carbin_cc_object instead")
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
    if (CARBIN_CC_OBJECT_DEPS)
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

################################################################################################
# carbin_cc_test
################################################################################################

function(carbin_cc_test)
    set(options
            DISABLED
            EXT
            EXCLUDE_SYSTEM
    )
    set(args NAME
            MODULE
    )
    set(list_args
            DEPS
            SOURCES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            INCLUDES
            COMMAND
            LINKS
    )

    cmake_parse_arguments(
            CARBIN_CC_TEST
            "${options}"
            "${args}"
            "${list_args}"
            ${ARGN}
    )
    if (NOT CARBIN_CC_TEST_MODULE)
        carbin_error("no module name")
    endif ()
    carbin_raw("-----------------------------------")
    carbin_print_label("Building Test" "${CARBIN_CC_TEST_NAME}")
    carbin_raw("-----------------------------------")

    set(${CARBIN_CC_TEST_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (CARBIN_CC_LIB_EXCLUDE_SYSTEM)
        set(${CARBIN_CC_TEST_NAME}_INCLUDE_SYSTEM "")
    endif ()

    if (VERBOSE_CARBIN_BUILD)
        carbin_print_list_label("Sources" CARBIN_CC_TEST_SOURCES)
        carbin_print_list_label("Deps" CARBIN_CC_TEST_DEPS)
        carbin_print_list_label("COPTS" CARBIN_CC_TEST_COPTS)
        carbin_print_list_label("Defines" CARBIN_CC_TEST_DEFINES)
        carbin_print_list_label("Links" CARBIN_CC_TEST_LINKS)
        message("-----------------------------------")
    endif ()
    set(CARBIN_RUN_THIS_TEST ON)
    if (CARBIN_CC_TEST_SKIP)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()
    if (CARBIN_CC_TEST_EXT)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()

    set(testcase ${CARBIN_CC_TEST_MODULE}_${CARBIN_CC_TEST_NAME})
    if (${CARBIN_CC_TEST_MODULE} IN_LIST ${PROJECT_NAME}_SKIP_TEST)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()

    add_executable(${testcase} ${CARBIN_CC_TEST_SOURCES})

    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:C>:${CARBIN_CC_TEST_COPTS}>)
    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CARBIN_CC_TEST_CXXOPTS}>)
    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CARBIN_CC_TEST_CUOPTS}>)
    if (CARBIN_CC_TEST_DEPS)
        add_dependencies(${testcase} ${CARBIN_CC_TEST_DEPS})
    endif ()
    target_link_libraries(${testcase} PRIVATE ${CARBIN_CC_TEST_LINKS})

    target_compile_definitions(${testcase}
            PUBLIC
            ${CARBIN_CC_TEST_DEFINES}
    )

    target_include_directories(${testcase} ${${CARBIN_CC_TEST_NAME}_INCLUDE_SYSTEM}
            PUBLIC
            ${CARBIN_CC_TEST_INCLUDES}
            "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
    if (NOT CARBIN_CC_TEST_COMMAND)
        set(CARBIN_CC_TEST_COMMAND ${testcase})
    endif ()

    if (CARBIN_RUN_THIS_TEST)
        add_test(NAME ${testcase}
                COMMAND ${CARBIN_CC_TEST_COMMAND})
    endif ()

endfunction()

function(carbin_cc_test_ext)
    set(options
            DISABLE
    )
    set(args NAME
            MODULE
            ALIAS
    )
    set(list_args
            ARGS
            FAIL_EXP
            SKIP_EXP
            PASS_EXP
    )

    cmake_parse_arguments(
            CARBIN_CC_TEST_EXT
            "${options}"
            "${args}"
            "${list_args}"
            ${ARGN}
    )

    set(CARBIN_RUN_THIS_TEST ON)
    if (CARBIN_CC_TEST_EXT_DISABLE)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()

    if (CARBIN_CC_TEST_EXT_MODULE)
        set(basecmd ${CARBIN_CC_TEST_EXT_MODULE}_${CARBIN_CC_TEST_EXT_NAME})
        if (${CARBIN_CC_TEST_EXT_MODULE} IN_LIST ${PROJECT_NAME}_SKIP_TEST)
            set(CARBIN_RUN_THIS_TEST OFF)
        endif ()
    else ()
        set(basecmd ${CARBIN_CC_TEST_EXT_NAME})
    endif ()

    if (CARBIN_CC_TEST_EXT_ALIAS)
        set(test_name ${CARBIN_CC_TEST_EXT_MODULE}_${CARBIN_CC_TEST_EXT_NAME}_${CARBIN_CC_TEST_EXT_ALIAS})
    else ()
        set(test_name ${CARBIN_CC_TEST_EXT_MODULE}_${CARBIN_CC_TEST_EXT_NAME})
    endif ()

    if (CARBIN_RUN_THIS_TEST)
        add_test(NAME ${test_name} COMMAND ${basecmd} ${CARBIN_CC_TEST_EXT_ARGS})
        if (CARBIN_CC_TEST_EXT_FAIL_EXP)
            set_property(TEST ${test_name} PROPERTY FAIL_REGULAR_EXPRESSION ${CARBIN_CC_TEST_EXT_FAIL_EXP})
        endif ()
        if (CARBIN_CC_TEST_EXT_PASS_EXP)
            set_property(TEST ${test_name} PROPERTY PASS_REGULAR_EXPRESSION ${CARBIN_CC_TEST_EXT_PASS_EXP})
        endif ()
        if (CARBIN_CC_TEST_EXT_SKIP_EXP)
            set_property(TEST ${test_name} PROPERTY SKIP_REGULAR_EXPRESSION ${CARBIN_CC_TEST_EXT_SKIP_EXP})
        endif ()
    endif ()

endfunction()

################################################################################################
# carbin_cc_binary
################################################################################################

function(carbin_cc_binary)
    set(options
            PUBLIC
            EXCLUDE_SYSTEM
    )
    set(list_args
            DEPS
            SOURCES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            LINKS
            INCLUDES
    )

    cmake_parse_arguments(
            CARBIN_CC_BINARY
            "${options}"
            "NAME"
            "${list_args}"
            ${ARGN}
    )

    set(${CARBIN_CC_BINARY_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (CARBIN_CC_LIB_EXCLUDE_SYSTEM)
        set(${CARBIN_CC_BINARY_NAME}_INCLUDE_SYSTEM "")
    endif ()

    carbin_raw("-----------------------------------")
    carbin_print_label("Building Binary" "${CARBIN_CC_BINARY_NAME}")
    carbin_raw("-----------------------------------")
    if (VERBOSE_CARBIN_BUILD)
        carbin_print_list_label("Sources" CARBIN_CC_BINARY_SOURCES)
        carbin_print_list_label("Deps" CARBIN_CC_BINARY_DEPS)
        carbin_print_list_label("COPTS" CARBIN_CC_BINARY_COPTS)
        carbin_print_list_label("CXXOPTS" CARBIN_CC_BINARY_CXXOPTS)
        carbin_print_list_label("CUOPTS" CARBIN_CC_BINARY_CUOPTS)
        carbin_print_list_label("Defines" CARBIN_CC_BINARY_DEFINES)
        carbin_print_list_label("Includes" CARBIN_CC_BINARY_INCLUDES)
        carbin_print_list_label("Links" CARBIN_CC_BINARY_LINKS)
        message("-----------------------------------")
    endif ()

    set(exec_case ${CARBIN_CC_BINARY_NAME})

    add_executable(${exec_case} ${CARBIN_CC_BINARY_SOURCES})

    target_compile_options(${exec_case} PRIVATE $<$<COMPILE_LANGUAGE:C>:${CARBIN_CC_BINARY_COPTS}>)
    target_compile_options(${exec_case} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CARBIN_CC_BINARY_CXXOPTS}>)
    target_compile_options(${exec_case} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CARBIN_CC_BINARY_CUOPTS}>)
    if (CARBIN_CC_BINARY_DEPS)
        add_dependencies(${exec_case} ${CARBIN_CC_BINARY_DEPS})
    endif ()
    target_link_libraries(${exec_case} PRIVATE ${CARBIN_CC_BINARY_LINKS})

    target_compile_definitions(${exec_case}
            PUBLIC
            ${CARBIN_CC_BINARY_DEFINES}
    )

    target_include_directories(${exec_case} ${${CARBIN_CC_LIB_NAME}_INCLUDE_SYSTEM}
            PRIVATE
            ${CARBIN_CC_BINARY_INCLUDES}
            "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${${PROJECT_NAME}_BINARY_DIR}>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
    if (CARBIN_CC_BINARY_PUBLIC)
        install(TARGETS ${exec_case}
                EXPORT ${PROJECT_NAME}Targets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif (CARBIN_CC_BINARY_PUBLIC)

endfunction()

################################################################################################
# carbin_cc_benchmark
################################################################################################

function(carbin_cc_bm)
    set(options
            DISABLED
            EXT
    )
    set(args NAME
            MODULE
    )
    set(list_args
            DEPS
            SOURCES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            INCLUDES
            COMMAND
            LINKS
    )

    cmake_parse_arguments(
            CARBIN_CC_BM
            "${options}"
            "${args}"
            "${list_args}"
            ${ARGN}
    )

    if (NOT CARBIN_CC_BM_MODULE)
        carbin_error("no module name to the bm")
    endif ()

    carbin_raw("-----------------------------------")
    carbin_print_label("Building Benchmark" "${CARBIN_CC_BM_NAME}")
    carbin_raw("-----------------------------------")
    if (VERBOSE_CARBIN_BUILD)
        carbin_print_list_label("Sources" CARBIN_CC_BM_SOURCES)
        carbin_print_list_label("Deps" CARBIN_CC_BM_DEPS)
        carbin_print_list_label("COPTS" CARBIN_CC_BM_COPTS)
        carbin_print_list_label("Defines" CARBIN_CC_BM_DEFINES)
        carbin_print_list_label("Liniks" CARBIN_CC_BM_LINKS)
        message("-----------------------------------")
    endif ()
    set(${CARBIN_CC_BM_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (CARBIN_CC_LIB_EXCLUDE_SYSTEM)
        set(${CARBIN_CC_BM_NAME}_INCLUDE_SYSTEM "")
    endif ()

    set(CARBIN_RUN_THIS_TEST ON)
    if (CARBIN_CC_BM_SKIP)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()
    if (CARBIN_CC_BM_EXT)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()
    set(testcase ${CARBIN_CC_BM_MODULE}_${CARBIN_CC_BM_NAME})
    if (${CARBIN_CC_BM_MODULE} IN_LIST ${PROJECT_NAME}_SKIP_BENCHMARK)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()

    add_executable(${testcase} ${CARBIN_CC_BM_SOURCES})

    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:C>:${CARBIN_CC_BM_COPTS}>)
    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CARBIN_CC_BM_CXXOPTS}>)
    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CARBIN_CC_BM_CUOPTS}>)
    if (CARBIN_CC_BM_DEPS)
        add_dependencies(${testcase} ${CARBIN_CC_BM_DEPS})
    endif ()
    target_link_libraries(${testcase} PRIVATE ${CARBIN_CC_BM_LINKS})
    target_compile_definitions(${testcase}
            PUBLIC
            ${CARBIN_CC_BM_DEFINES}
    )

    target_include_directories(${testcase} ${${CARBIN_CC_BM_NAME}_INCLUDE_SYSTEM}
            PUBLIC
            ${CARBIN_CC_BM_INCLUDES}
            "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
    if (NOT CARBIN_CC_BM_COMMAND)
        set(CARBIN_CC_BM_COMMAND ${testcase})
    endif ()

    if (CARBIN_RUN_THIS_TEST)
        add_test(NAME ${testcase}
                COMMAND ${CARBIN_CC_BM_COMMAND})
    endif ()

endfunction()

function(carbin_cc_bm_ext)
    set(options
            DISABLE
    )
    set(args NAME
            MODULE
            ALIAS
    )
    set(list_args
            ARGS
            FAIL_EXP
            SKIP_EXP
            PASS_EXP
    )

    cmake_parse_arguments(
            CARBIN_CC_BM_EXT
            "${options}"
            "${args}"
            "${list_args}"
            ${ARGN}
    )

    set(CARBIN_RUN_THIS_TEST ON)
    if (CARBIN_CC_BM_EXT_DISABLE)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()

    if (CARBIN_CC_BM_EXT_MODULE)
        set(basecmd ${CARBIN_CC_BM_EXT_MODULE}_${CARBIN_CC_BM_EXT_NAME})
        if (${CARBIN_CC_BM_EXT_MODULE} IN_LIST ${PROJECT_NAME}_SKIP_BENCHMARK)
            set(CARBIN_RUN_THIS_TEST OFF)
        endif ()
    else ()
        set(basecmd ${CARBIN_CC_BM_EXT_NAME})
    endif ()

    if (CARBIN_CC_BM_EXT_ALIAS)
        set(test_name ${CARBIN_CC_BM_EXT_MODULE}_${CARBIN_CC_BM_EXT_NAME}_${CARBIN_CC_BM_EXT_ALIAS})
    else ()
        set(test_name ${CARBIN_CC_BM_EXT_MODULE}_${CARBIN_CC_BM_EXT_NAME})
    endif ()

    if (CARBIN_RUN_THIS_TEST)
        add_test(NAME ${test_name} COMMAND ${basecmd} ${CARBIN_CC_BM_EXT_ARGS})
        if (CARBIN_CC_BM_EXT_FAIL_EXP)
            set_property(TEST ${test_name} PROPERTY FAIL_REGULAR_EXPRESSION ${CARBIN_CC_BM_EXT_FAIL_EXP})
        endif ()
        if (CARBIN_CC_BM_EXT_PASS_EXP)
            set_property(TEST ${test_name} PROPERTY PASS_REGULAR_EXPRESSION ${CARBIN_CC_BM_EXT_PASS_EXP})
        endif ()
        if (CARBIN_CC_BM_EXT_SKIP_EXP)
            set_property(TEST ${test_name} PROPERTY SKIP_REGULAR_EXPRESSION ${CARBIN_CC_BM_EXT_SKIP_EXP})
        endif ()
    endif ()

endfunction()

################################################################################################
# carbin_target_check
################################################################################################
function(carbin_target_check my_target)
    if (NOT TARGET ${my_target})
        message(FATAL_ERROR " CARBIN: compiling ${PROJECT_NAME} requires a ${my_target} CMake target in your project,
                   see CMake/README.md for more details")
    endif (NOT TARGET ${my_target})
endfunction()

################################################################################################
# carbin_simd
################################################################################################
INCLUDE(CheckCXXSourceRuns)

SET(SSE1_CODE "
  #include <xmmintrin.h>

  int main()
  {
    __m128 a;
    float vals[4] = {0,0,0,0};
    a = _mm_loadu_ps(vals);  // SSE1
    return 0;
  }")

SET(SSE2_CODE "
  #include <emmintrin.h>

  int main()
  {
    __m128d a;
    double vals[2] = {0,0};
    a = _mm_loadu_pd(vals);  // SSE2
    return 0;
  }")

SET(SSE3_CODE "
#include <pmmintrin.h>
int main() {
    __m128 u, v;
    u = _mm_set1_ps(0.0f);
    v = _mm_moveldup_ps(u); // SSE3
    return 0;
}")

SET(SSSE3_CODE "
  #include <tmmintrin.h>
  const double v = 0;
  int main() {
    __m128i a = _mm_setzero_si128();
    __m128i b = _mm_abs_epi32(a); // SSSE3
    return 0;
  }")

SET(SSE4_1_CODE "
  #include <smmintrin.h>

  int main ()
  {
    __m128i a = {0,0,0,0}, b = {0,0,0,0};
    __m128i res = _mm_max_epi8(a, b); // SSE4_1

    return 0;
  }
")

SET(SSE4_2_CODE "
  #include <nmmintrin.h>

  int main()
  {
    __m128i a = {0,0,0,0}, b = {0,0,0,0}, c = {0,0,0,0};
    c = _mm_cmpgt_epi64(a, b);  // SSE4_2
    return 0;
  }
")


MACRO(CHECK_SSE lang type flags)
    SET(__FLAG_I 1)
    SET(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
    FOREACH (__FLAG ${flags})
        IF (NOT ${lang}_${type}_FOUND)
            SET(CMAKE_REQUIRED_FLAGS ${__FLAG})
            CHECK_CXX_SOURCE_RUNS("${${type}_CODE}" ${lang}_HAS_${type}_${__FLAG_I})
            IF (${lang}_HAS_${type}_${__FLAG_I})
                SET(${lang}_${type}_FOUND TRUE)
            ENDIF ()
            MATH(EXPR __FLAG_I "${__FLAG_I}+1")
        ENDIF ()
    ENDFOREACH ()
    SET(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})

    IF (NOT ${lang}_${type}_FOUND)
        SET(${lang}_${type}_FOUND FALSE)
    ENDIF ()
    MARK_AS_ADVANCED(${lang}_${type}_FOUND ${lang}_${type}_FLAGS)

ENDMACRO()

SET(AVX_CODE "
#if !defined __AVX__ // MSVC supports this flag since MSVS 2013
#error \"__AVX__ define is missing\"
#endif
#include <immintrin.h>
void test()
{
    __m256 a = _mm256_set1_ps(0.0f);
}
int main() { return 0; }")

SET(AVX2_CODE "
#if !defined __AVX2__ // MSVC supports this flag since MSVS 2013
#error \"__AVX2__ define is missing\"
#endif
#include <immintrin.h>
void test()
{
    int data[8] = {0,0,0,0, 0,0,0,0};
    __m256i a = _mm256_loadu_si256((const __m256i *)data);
    __m256i b = _mm256_bslli_epi128(a, 1);  // available in GCC 4.9.3+
}
int main() { return 0; }")

SET(AVX512_CODE "
#if defined __AVX512__ || defined __AVX512F__
#include <immintrin.h>
void test()
{
    __m512i zmm = _mm512_setzero_si512();
#if defined __GNUC__ && defined __x86_64__
    asm volatile (\"\" : : : \"zmm16\", \"zmm17\", \"zmm18\", \"zmm19\");
#endif
}
#else
#error \"AVX512 is not supported\"
#endif
int main() { return 0; }")


MACRO(CHECK_AVX type flags)
    SET(__FLAG_I 1)
    SET(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
    FOREACH (__FLAG ${flags})
        IF (NOT CXX_${type}_FOUND)
            SET(CMAKE_REQUIRED_FLAGS ${__FLAG})
            CHECK_CXX_SOURCE_RUNS("${${type}_CODE}" CXX_HAS_${type}_${__FLAG_I})
            IF (CXX_HAS_${type}_${__FLAG_I})
                SET(CXX_${type}_FOUND TRUE)
            ENDIF ()
            MATH(EXPR __FLAG_I "${__FLAG_I}+1")
        ENDIF ()
    ENDFOREACH ()
    SET(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})

    IF (NOT CXX_${type}_FOUND)
        SET(CXX_${type}_FOUND FALSE)
    ENDIF ()

    MARK_AS_ADVANCED(CXX_${type}_FOUND CXX_${type}_FLAGS)

ENDMACRO()
if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
    CHECK_AVX("AVX" ";-mavx;/arch:AVX")
    CHECK_AVX("AVX2" ";-mavx2;/arch:AVX2")
    CHECK_AVX("AVX512" ";-mavx512;/arch:AVX512")
    CHECK_SSE(CXX "SSE1" ";-msse;/arch:SSE")
    CHECK_SSE(CXX "SSE2" ";-msse2;/arch:SSE2")
    CHECK_SSE(CXX "SSE3" ";-msse3;/arch:SSE3")
    CHECK_SSE(CXX "SSSE3" ";-mssse3;/arch:SSSE3")
    CHECK_SSE(CXX "SSE4_1" ";-msse4.1;-msse4;/arch:SSE4")
    CHECK_SSE(CXX "SSE4_2" ";-msse4.2;-msse4;/arch:SSE4")
elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm.*|aarch64")
    IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
        execute_process(COMMAND cat /proc/cpuinfo OUTPUT_VARIABLE CPUINFO)

        #neon instruction can be found on the majority part of modern ARM processor
        STRING(REGEX REPLACE "^.*(neon).*$" "\\1" NEON_THERE ${CPUINFO})
        STRING(COMPARE EQUAL "neon" "${NEON_THERE}" NEON_TRUE)
        IF (NEON_TRUE)
            set(NEON_FOUND true BOOL "NEON available on host")
        ELSE ()
            set(NEON_FOUND false BOOL "NEON available on host")
        ENDIF ()

        #Find the processor type (for now OMAP3 or OMAP4)
        STRING(REGEX REPLACE "^.*(OMAP3).*$" "\\1" OMAP3_THERE ${CPUINFO})
        STRING(COMPARE EQUAL "OMAP3" "${OMAP3_THERE}" OMAP3_TRUE)
        IF (OMAP3_TRUE)
            set(CORTEXA8_FOUND true BOOL "OMAP3 available on host")
        ELSE ()
            set(CORTEXA8_FOUND false BOOL "OMAP3 available on host")
        ENDIF ()

        #Find the processor type (for now OMAP3 or OMAP4)
        STRING(REGEX REPLACE "^.*(OMAP4).*$" "\\1" OMAP4_THERE ${CPUINFO})
        STRING(COMPARE EQUAL "OMAP4" "${OMAP4_THERE}" OMAP4_TRUE)
        IF (OMAP4_TRUE)
            set(CORTEXA9_FOUND true BOOL "OMAP4 available on host")
        ELSE ()
            set(CORTEXA9_FOUND false BOOL "OMAP4 available on host")
        ENDIF ()

    ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        execute_process(COMMAND /usr/sbin/sysctl -n machdep.cpu.features OUTPUT_VARIABLE CPUINFO)

        #neon instruction can be found on the majority part of modern ARM processor
        STRING(REGEX REPLACE "^.*(neon).*$" "\\1" NEON_THERE ${CPUINFO})
        STRING(COMPARE EQUAL "neon" "${NEON_THERE}" NEON_TRUE)
        IF (NEON_TRUE)
            set(NEON_FOUND true BOOL "NEON available on host")
        ELSE ()
            set(NEON_FOUND false BOOL "NEON available on host")
        ENDIF ()

    ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
        # TODO
        set(CORTEXA8_FOUND false BOOL "OMAP3 not available on host")
        set(CORTEXA9_FOUND false BOOL "OMAP4 not available on host")
        set(NEON_FOUND false BOOL "NEON not available on host")
    ELSE (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(CORTEXA8_FOUND false BOOL "OMAP3 not available on host")
        set(CORTEXA9_FOUND false BOOL "OMAP4 not available on host")
        set(NEON_FOUND false BOOL "NEON not available on host")
    ENDIF ()

    if (NOT NEON_FOUND)
        MESSAGE(STATUS "Could not find hardware support for NEON on this machine.")
    endif ()
    if (NOT CORTEXA8_FOUND)
        MESSAGE(STATUS "No OMAP3 processor on this on this machine.")
    endif ()
    if (NOT CORTEXA9_FOUND)
        MESSAGE(STATUS "No OMAP4 processor on this on this machine.")
    endif ()
    mark_as_advanced(NEON_FOUND)
endif ()
################################################################################################
# out of source build
################################################################################################
macro(CARBIN_ENSURE_OUT_OF_SOURCE_BUILD errorMessage)

    string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" is_insource)
    if (is_insource)
        carbin_error(${errorMessage} "In-source builds are not allowed.
    CMake would overwrite the makefiles distributed with Compiler-RT.
    Please create a directory and run cmake from there, passing the path
    to this source directory as the last argument.
    This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
    Please delete them.")

    endif (is_insource)

endmacro(CARBIN_ENSURE_OUT_OF_SOURCE_BUILD)

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
    link_directories($ENV{CONDA_PREFIX}/${CMAKE_INSTALL_LIBDIR})
endif ()

if (CARBIN_DEPS_ENABLE)
    list(APPEND CMAKE_PREFIX_PATH ${CARBIN_DEPS_PREFIX})
    include_directories(${CARBIN_DEPS_PREFIX}/include)
    link_directories(${CARBIN_DEPS_PREFIX}/${CMAKE_INSTALL_LIBDIR})
endif ()

if (CARBIN_INSTALL_LIB)
    set(CMAKE_INSTALL_LIBDIR lib)
endif ()

if (CARBIN_USE_SYSTEM_INCLUDES)
    set(CARBIN_INTERNAL_INCLUDE_WARNING_GUARD SYSTEM)
else ()
    set(CARBIN_INTERNAL_INCLUDE_WARNING_GUARD "")
endif ()

CARBIN_ENSURE_OUT_OF_SOURCE_BUILD("must out of source dir")

#if (NOT DEV_MODE AND ${PROJECT_VERSION} MATCHES "0.0.0")
#    carbin_error("PROJECT_VERSION must be set in file project_profile or set -DDEV_MODE=true for develop debug")
#endif()
################################################################################################
# builtin find_package
################################################################################################
##################################
# benchmark
##################################
macro(carbin_require_benchmark)
    find_path(BENCHMARK_INCLUDE_PATH NAMES benchmark/benchmark.h)
    find_library(BENCHMARK_LIB NAMES libbenchmark.a benchmark)
    find_library(BENCHMARK_MAIN_LIB NAMES libbenchmark_main.a benchmark_main)
    if ((NOT BENCHMARK_INCLUDE_PATH) OR (NOT BENCHMARK_LIB) OR (NOT BENCHMARK_MAIN_LIB))
        carbin_error("Fail to find benchmark")
    endif ()
endmacro()

##################################
# doctest
##################################
macro(carbin_require_doctest)
    find_path(DOCTEST_INCLUDE_PATH NAMES doctest/doctest.h)
    if ((NOT DOCTEST_INCLUDE_PATH))
        carbin_error("Fail to find doctest")
    endif ()
endmacro()
##################################
# gtest
##################################
macro(carbin_require_gtest)
    find_path(GTEST_INCLUDE_PATH NAMES gtest/gtest.h)
    find_library(GTEST_LIB NAMES libgtest.a gtest)
    find_library(GTEST_MAIN_LIB NAMES libgtest_main.a gtest_main)
    if ((NOT GTEST_INCLUDE_PATH) OR (NOT GTEST_LIB) OR (NOT GTEST_MAIN_LIB))
        carbin_error("Fail to find gtest")
    endif ()
endmacro()
##################################
# leveldb
##################################
macro(carbin_find_leveldb)
    set(LEVELDB_FOUND FALSE)
    set(LEVELDB_DEV_FOUND FALSE)
    set(LEVELDB_SHARED_FOUND FALSE)
    set(LEVELDB_STATIC_FOUND FALSE)
    find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
    find_library(LEVELDB_SHARED_LIB NAMES leveldb)
    find_library(LEVELDB_STATIC_LIB NAMES libleveldb.a)
    if (LEVELDB_STATIC_LIB)
        set(LEVELDB_STATIC_FOUND TRUE)
    endif ()
    if (LEVELDB_SHARED_LIB)
        set(LEVELDB_SHARED_FOUND TRUE)
    endif ()
    if (LEVELDB_SHARED_FOUND OR LEVELDB_STATIC_FOUND)
        set(LEVELDB_FOUND TRUE)
    endif ()

    if (LEVELDB_INCLUDE_PATH AND LEVELDB_FOUND)
        set(LEVELDB_DEV_FOUND TRUE)
    endif ()
    carbin_print("LEVELDB_FOUND: ${LEVELDB_FOUND}")
    carbin_print("LEVELDB_DEV_FOUND: ${LEVELDB_DEV_FOUND}")
    if (LEVELDB_FOUND)
        carbin_print("LEVELDB_INCLUDE_PATH: ${LEVELDB_INCLUDE_PATH}")
        carbin_print("LEVELDB_SHARED_LIB: ${LEVELDB_SHARED_LIB}")
        carbin_print("LEVELDB_STATIC_LIB: ${LEVELDB_STATIC_LIB}")
    endif ()
endmacro()

##################################
# gflags
##################################
macro(carbin_find_gflags)
    set(GFLAGS_FOUND FALSE)
    set(GFLAGS_DEV_FOUND FALSE)
    set(GFLAGS_SHARED_FOUND FALSE)
    set(GFLAGS_STATIC_FOUND FALSE)
    find_path(GFLAGS_INCLUDE_PATH NAMES gflags/gflags.h)
    find_library(GFLAGS_SHARED_LIB NAMES gflags)
    find_library(GFLAGS_STATIC_LIB NAMES libgflags)
    if (GFLAGS_STATIC_LIB)
        set(GFLAGS_STATIC_FOUND TRUE)
    endif ()
    if (GFLAGS_SHARED_LIB)
        set(GFLAGS_SHARED_FOUND TRUE)
    endif ()
    if (GFLAGS_SHARED_FOUND OR GFLAGS_STATIC_FOUND)
        set(GFLAGS_FOUND TRUE)
    endif ()

    if (GFLAGS_INCLUDE_PATH AND GFLAGS_FOUND)
        set(GFLAGS_DEV_FOUND TRUE)
    endif ()
    carbin_print("GFLAGS_FOUND: ${GFLAGS_FOUND}")
    carbin_print("GFLAGS_DEV_FOUND: ${GFLAGS_DEV_FOUND}")
    if (GFLAGS_FOUND)
        carbin_print("GFLAGS_INCLUDE_PATH: ${GFLAGS_INCLUDE_PATH}")
        carbin_print("GFLAGS_SHARED_LIB: ${GFLAGS_SHARED_LIB}")
        carbin_print("GFLAGS_STATIC_LIB: ${GFLAGS_STATIC_LIB}")
    endif ()
endmacro()

##################################
# rocksdb
##################################
macro(carbin_find_rocksdb)
    set(ROCKSDB_FOUND FALSE)
    set(ROCKSDB_DEV_FOUND FALSE)
    set(ROCKSDB_SHARED_FOUND FALSE)
    set(ROCKSDB_STATIC_FOUND FALSE)
    find_path(ROCKSDB_INCLUDE_PATH NAMES rocksdb/db.h)
    find_library(ROCKSDB_SHARED_LIB NAMES rocksdb)
    find_library(ROCKSDB_STATIC_LIB NAMES librocksdb.a)
    if (ROCKSDB_STATIC_LIB)
        set(ROCKSDB_STATIC_FOUND TRUE)
    endif ()
    if (ROCKSDB_SHARED_LIB)
        set(ROCKSDB_SHARED_FOUND TRUE)
    endif ()
    if (ROCKSDB_SHARED_FOUND OR ROCKSDB_STATIC_FOUND)
        set(ROCKSDB_FOUND TRUE)
    endif ()

    if (ROCKSDB_INCLUDE_PATH AND ROCKSDB_FOUND)
        set(ROCKSDB_DEV_FOUND TRUE)
    endif ()
    carbin_print("ROCKSDB_FOUND: ${ROCKSDB_FOUND}")
    carbin_print("ROCKSDB_DEV_FOUND: ${ROCKSDB_DEV_FOUND}")
    if (ROCKSDB_FOUND)
        carbin_print("ROCKSDB_INCLUDE_PATH: ${ROCKSDB_INCLUDE_PATH}")
        carbin_print("ROCKSDB_SHARED_LIB: ${ROCKSDB_SHARED_LIB}")
        carbin_print("ROCKSDB_STATIC_LIB: ${ROCKSDB_STATIC_LIB}")
    endif ()
endmacro()

##################################
# snappy
##################################
macro(carbin_find_snappy)
    set(SNAPPY_FOUND FALSE)
    set(SNAPPY_DEV_FOUND FALSE)
    set(SNAPPY_SHARED_FOUND FALSE)
    set(SNAPPY_STATIC_FOUND FALSE)
    find_path(SNAPPY_INCLUDE_PATH NAMES snappy.h)
    find_library(SNAPPY_SHARED_LIB NAMES snappy)
    find_library(SNAPPY_STATIC_LIB NAMES libsnappy.a)
    if (SNAPPY_STATIC_LIB)
        set(SNAPPY_STATIC_FOUND TRUE)
    endif ()
    if (SNAPPY_SHARED_LIB)
        set(SNAPPY_SHARED_FOUND TRUE)
    endif ()
    if (SNAPPY_SHARED_FOUND OR SNAPPY_STATIC_FOUND)
        set(SNAPPY_FOUND TRUE)
    endif ()

    if (SNAPPY_INCLUDE_PATH AND SNAPPY_FOUND)
        set(SNAPPY_DEV_FOUND TRUE)
    endif ()
endmacro()

##################################
# zlib
##################################
macro(carbin_find_zlib)
    set(ZLIB_FOUND FALSE)
    set(ZLIB_DEV_FOUND FALSE)
    set(ZLIB_SHARED_FOUND FALSE)
    set(ZLIB_STATIC_FOUND FALSE)
    find_path(ZLIB_INCLUDE_PATH NAMES zlib.h)
    find_library(ZLIB_SHARED_LIB NAMES z)
    find_library(ZLIB_STATIC_LIB NAMES libz.a)
    if (ZLIB_STATIC_LIB)
        set(ZLIB_STATIC_FOUND TRUE)
    endif ()
    if (ZLIB_SHARED_LIB)
        set(ZLIB_SHARED_FOUND TRUE)
    endif ()
    if (ZLIB_SHARED_FOUND OR ZLIB_STATIC_FOUND)
        set(ZLIB_FOUND TRUE)
    endif ()

    if (ZLIB_INCLUDE_PATH AND ZLIB_FOUND)
        set(ZLIB_DEV_FOUND TRUE)
    endif ()
    carbin_print("ZLIB_FOUND: ${ZLIB_FOUND}")
    carbin_print("ZLIB_DEV_FOUND: ${ZLIB_DEV_FOUND}")
    if (ZLIB_FOUND)
        carbin_print("ZLIB_INCLUDE_PATH: ${ZLIB_INCLUDE_PATH}")
        carbin_print("ZLIB_SHARED_LIB: ${ZLIB_SHARED_LIB}")
        carbin_print("ZLIB_STATIC_LIB: ${ZLIB_STATIC_LIB}")
    endif ()
endmacro()

##################################
# lz4
##################################
macro(carbin_find_lz4)
    set(LZ4_FOUND FALSE)
    set(LZ4_DEV_FOUND FALSE)
    set(LZ4_SHARED_FOUND FALSE)
    set(LZ4_STATIC_FOUND FALSE)
    find_path(LZ4_INCLUDE_PATH NAMES lz4.h)
    find_library(LZ4_SHARED_LIB NAMES lz4)
    find_library(LZ4_STATIC_LIB NAMES liblz4.a)
    if (LZ4_STATIC_LIB)
        set(LZ4_STATIC_FOUND TRUE)
    endif ()
    if (LZ4_SHARED_LIB)
        set(LZ4_SHARED_FOUND TRUE)
    endif ()
    if (LZ4_SHARED_FOUND OR LZ4_STATIC_FOUND)
        set(LZ4_FOUND TRUE)
    endif ()

    if (LZ4_INCLUDE_PATH AND LZ4_FOUND)
        set(LZ4_DEV_FOUND TRUE)
    endif ()
endmacro()

##################################
# bzip2
##################################
macro(carbin_find_bzip2)
    set(BZIP2_FOUND FALSE)
    set(BZIP2_DEV_FOUND FALSE)
    set(BZIP2_SHARED_FOUND FALSE)
    set(BZIP2_STATIC_FOUND FALSE)
    find_path(BZIP2_INCLUDE_PATH NAMES bzlib.h)
    find_library(BZIP2_SHARED_LIB NAMES bz2)
    find_library(BZIP2_STATIC_LIB NAMES libbz2.a)
    if (BZIP2_STATIC_LIB)
        set(BZIP2_STATIC_FOUND TRUE)
    endif ()
    if (BZIP2_SHARED_LIB)
        set(BZIP2_SHARED_FOUND TRUE)
    endif ()
    if (BZIP2_SHARED_FOUND OR BZIP2_STATIC_FOUND)
        set(BZIP2_FOUND TRUE)
    endif ()

    if (BZIP2_INCLUDE_PATH AND BZIP2_FOUND)
        set(BZIP2_DEV_FOUND TRUE)
    endif ()
endmacro()

##################################
# lzma
##################################
macro(carbin_find_lzma)
    set(LZMA_FOUND FALSE)
    set(LZMA_DEV_FOUND FALSE)
    set(LZMA_SHARED_FOUND FALSE)
    set(LZMA_STATIC_FOUND FALSE)
    find_path(LZMA_INCLUDE_PATH NAMES lzma.h)
    find_library(LZMA_SHARED_LIB NAMES lzma)
    find_library(LZMA_STATIC_LIB NAMES liblzma.a)
    if (LZMA_STATIC_LIB)
        set(LZMA_STATIC_FOUND TRUE)
    endif ()
    if (LZMA_SHARED_LIB)
        set(LZMA_SHARED_FOUND TRUE)
    endif ()
    if (LZMA_SHARED_FOUND OR LZMA_STATIC_FOUND)
        set(LZMA_FOUND TRUE)
    endif ()

    if (LZMA_INCLUDE_PATH AND LZMA_FOUND)
        set(LZMA_DEV_FOUND TRUE)
    endif ()
endmacro()
