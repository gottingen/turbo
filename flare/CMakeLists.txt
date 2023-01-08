


file(GLOB_RECURSE FLARE_BASE_SRC "base/*.cc")
file(GLOB_RECURSE FLARE_BOOTSTRAP_SRC "bootstrap/*.cc")
file(GLOB_RECURSE FLARE_CONTAINER_SRC "container/*.cc")
file(GLOB_RECURSE FLARE_DAG_SRC "dag/*.cc")
file(GLOB_RECURSE FLARE_DEBUG_SRC "debugging/*.cc")
file(GLOB_RECURSE FLARE_FIBER_SRC "fiber/*.cc")
file(GLOB_RECURSE FLARE_FILES_SRC "files/*.cc")
file(GLOB_RECURSE FLARE_HASH_SRC "hash/*.cc")
file(GLOB_RECURSE FLARE_IO_SRC "io/*.cc")
file(GLOB_RECURSE FLARE_LOG_SRC "log/*.cc")
file(GLOB_RECURSE FLARE_MEMORY_SRC "memory/*.cc")
file(GLOB_RECURSE FLARE_METRICS_SRC "metrics/*.cc")
file(GLOB_RECURSE FLARE_JSON_SRC "rapidjson/*.cc")
file(GLOB_RECURSE FLARE_STRINGS_SRC "strings/*.cc")
file(GLOB_RECURSE FLARE_SYSTEM_SRC "system/*.cc")
file(GLOB_RECURSE FLARE_TIMES_SRC "times/*.cc")
file(GLOB_RECURSE FLARE_THREAD_SRC "thread/*.cc")

set(FLARE_SRC
        "${PROJECT_SOURCE_DIR}/flare/base/dynamic_annotations/dynamic_annotations.c"
        ${FLARE_BASE_SRC}
        ${FLARE_CONTAINER_SRC}
        ${FLARE_HASH_SRC}
        ${FLARE_METRICS_SRC}
        ${FLARE_MEMORY_SRC}
        ${FLARE_IO_SRC}
        ${FLARE_STRINGS_SRC}
        ${FLARE_DEBUG_SRC}
        ${FLARE_LOG_SRC}
        ${FLARE_BOOTSTRAP_SRC}
        ${FLARE_HASH_SRC}
        ${FLARE_TIMES_SRC}
        ${FLARE_FIBER_SRC}
        ${FLARE_FILES_SRC}
        ${FLARE_THREAD_SRC}
        ${FLARE_FUTURE_SRC}
        ${FLARE_DAG_SRC}
        ${FLARE_SYSTEM_SRC}
        ${FLARE_JSON_SRC}
        )

#
#if (APPLE)
#    find_library(CoreFoundation CoreFoundation)
#    carbin_cc_library(
#            NAME
#            abel
#            SRCS
#            ${ABEL_SRC}
#            COPTS
#            ${CARBIN_DEFAULT_COPTS}
#            DEPS
#                ${CoreFoundation}
#                pthread
#            PUBLIC
#    )
#elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
#    carbin_cc_library(
#            NAME
#            abel
#            SRCS
#            ${ABEL_SRC}
#            COPTS
#            ${CARBIN_DEFAULT_COPTS}
#            DEPS
#            rt
#            pthread
#            PUBLIC
#    )
#endif ()
#

# carbin_cc_library(  NAME myLibrary
#                  NAMESPACE myNamespace
#                  SOURCES
#                       myLib.cc
#                       myLib_functions.cc
#                  HEADERS
#                        mylib.h
#                  PUBLIC_DEFINITIONS
#                     USE_DOUBLE_PRECISION=1
#                  PRIVATE_DEFINITIONS
#                     DEBUG_VERBOSE
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
#                  PUBLIC
#                  SHARED

link_directories($ENV{CONDA_PREFIX}/lib)
carbin_cc_library(
        NAMESPACE flare
        NAME flare
        SOURCES ${FLARE_SRC}
        PUBLIC_LINKED_TARGETS ${CARBIN_DYLINK} ${DYNAMIC_LIB}
        PRIVATE_COMPILE_OPTIONS ${CARBIN_DEFAULT_COPTS}
        PUBLIC
        SHARED
)

if (ENABLE_TESTING)
    add_definitions(
            -DUNIT_TEST
    )
    carbin_cc_library(
            NAMESPACE flare
            NAME flare-test
            SOURCES ${FLARE_SRC}
            PUBLIC_LINKED_TARGETS ${CARBIN_DYLINK} ${DYNAMIC_LIB}
            PRIVATE_COMPILE_OPTIONS ${CARBIN_DEFAULT_COPTS}
            PUBLIC
            SHARED
    )
endif ()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/flare
        DESTINATION ${CMAKE_INSTALL_PREFIX}/flare
        FILES_MATCHING
        PATTERN "*.h"
        )


