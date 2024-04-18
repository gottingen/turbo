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
    if(obj_len EQUAL -1)
        carbin_error("no source or object give to the library ${CARBIN_CC_LIB_NAME}")
    endif ()
    add_library(${CARBIN_CC_LIB_NAME}_static STATIC  ${CARBIN_CC_LIB_OBJECTS_FLATTEN})
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

    add_library(${CARBIN_CC_LIB_NAME}_shared SHARED  ${CARBIN_CC_LIB_OBJECTS_FLATTEN})
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
    if(obj_len EQUAL -1)
        carbin_error("no source or object give to the library ${CARBIN_CC_LIB_NAME}")
    endif ()
    add_library(${CARBIN_CC_LIB_NAME}_static STATIC  ${CARBIN_CC_LIB_OBJECTS_FLATTEN})
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
        add_library(${CARBIN_CC_LIB_NAME}_shared SHARED  ${CARBIN_CC_LIB_OBJECTS_FLATTEN})
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

