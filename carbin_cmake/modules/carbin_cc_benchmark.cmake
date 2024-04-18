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

    if(NOT CARBIN_CC_BM_MODULE)
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
    if(${CARBIN_CC_BM_MODULE} IN_LIST ${PROJECT_NAME}_SKIP_BENCHMARK)
        set(CARBIN_RUN_THIS_TEST OFF)
    endif ()

    add_executable(${testcase} ${CARBIN_CC_BM_SOURCES})

    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:C>:${CARBIN_CC_BM_COPTS}>)
    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CARBIN_CC_BM_CXXOPTS}>)
    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CARBIN_CC_BM_CUOPTS}>)
    if(CARBIN_CC_BM_DEPS)
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

    if(CARBIN_CC_BM_EXT_MODULE)
        set(basecmd ${CARBIN_CC_BM_EXT_MODULE}_${CARBIN_CC_BM_EXT_NAME})
        if(${CARBIN_CC_BM_EXT_MODULE} IN_LIST ${PROJECT_NAME}_SKIP_BENCHMARK)
            set(CARBIN_RUN_THIS_TEST OFF)
        endif ()
    else ()
        set(basecmd ${CARBIN_CC_BM_EXT_NAME})
    endif()

    if (CARBIN_CC_BM_EXT_ALIAS)
        set(test_name ${CARBIN_CC_BM_EXT_MODULE}_${CARBIN_CC_BM_EXT_NAME}_${CARBIN_CC_BM_EXT_ALIAS})
    else ()
        set(test_name ${CARBIN_CC_BM_EXT_MODULE}_${CARBIN_CC_BM_EXT_NAME})
    endif ()

    if(CARBIN_RUN_THIS_TEST)
        add_test(NAME ${test_name} COMMAND ${basecmd} ${CARBIN_CC_BM_EXT_ARGS})
        if(CARBIN_CC_BM_EXT_FAIL_EXP)
            set_property(TEST ${test_name} PROPERTY FAIL_REGULAR_EXPRESSION ${CARBIN_CC_BM_EXT_FAIL_EXP})
        endif ()
        if(CARBIN_CC_BM_EXT_PASS_EXP)
            set_property(TEST ${test_name} PROPERTY PASS_REGULAR_EXPRESSION ${CARBIN_CC_BM_EXT_PASS_EXP})
        endif ()
        if(CARBIN_CC_BM_EXT_SKIP_EXP)
            set_property(TEST ${test_name} PROPERTY SKIP_REGULAR_EXPRESSION ${CARBIN_CC_BM_EXT_SKIP_EXP})
        endif ()
    endif ()

endfunction()