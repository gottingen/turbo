


file(GLOB_RECURSE TURBO_BASE_SRC "base/*.cc")
file(GLOB_RECURSE TURBO_BOOTSTRAP_SRC "bootstrap/*.cc")
file(GLOB_RECURSE TURBO_CONTAINER_SRC "container/*.cc")
file(GLOB_RECURSE TURBO_DEBUG_SRC "debugging/*.cc")
file(GLOB_RECURSE TURBO_FILES_SRC "files/*.cc")
file(GLOB_RECURSE TURBO_HASH_SRC "hash/*.cc")
file(GLOB_RECURSE TURBO_IO_SRC "io/*.cc")
file(GLOB_RECURSE TURBO_LOG_SRC "log/*.cc")
file(GLOB_RECURSE TURBO_MEMORY_SRC "memory/*.cc")
file(GLOB_RECURSE TURBO_JSON_SRC "rapidjson/*.cc")
file(GLOB_RECURSE TURBO_STRINGS_SRC "strings/*.cc")
file(GLOB_RECURSE TURBO_SYSTEM_SRC "system/*.cc")
file(GLOB_RECURSE TURBO_TIMES_SRC "times/*.cc")
file(GLOB_RECURSE TURBO_THREAD_SRC "thread/*.cc")

set(TURBO_SRC
        "${PROJECT_SOURCE_DIR}/turbo/base/dynamic_annotations/dynamic_annotations.c"
        ${TURBO_BASE_SRC}
        ${TURBO_CONTAINER_SRC}
        ${TURBO_HASH_SRC}
        ${TURBO_METRICS_SRC}
        ${TURBO_MEMORY_SRC}
        ${TURBO_IO_SRC}
        ${TURBO_STRINGS_SRC}
        ${TURBO_DEBUG_SRC}
        ${TURBO_LOG_SRC}
        ${TURBO_BOOTSTRAP_SRC}
        ${TURBO_HASH_SRC}
        ${TURBO_TIMES_SRC}
        ${TURBO_FILES_SRC}
        ${TURBO_THREAD_SRC}
        ${TURBO_SYSTEM_SRC}
        ${TURBO_JSON_SRC}
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

carbin_cc_library(
        NAMESPACE turbo
        NAME turbo
        SOURCES ${TURBO_SRC}
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
            NAMESPACE turbo
            NAME turbo-test
            SOURCES ${TURBO_SRC}
            PUBLIC_LINKED_TARGETS ${CARBIN_DYLINK} ${DYNAMIC_LIB}
            PRIVATE_COMPILE_OPTIONS ${CARBIN_DEFAULT_COPTS}
            PUBLIC
            SHARED
    )
endif ()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/turbo
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        FILES_MATCHING
        PATTERN "*.h"
        )


