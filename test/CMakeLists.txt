
include_directories(${PROJECT_SOURCE_DIR}/test)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include(CompileProto)
set(TEST_PROTO_FILES
        cord_buf.proto
 )
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/hdrs)
set(PROTOC_FLAGS ${PROTOC_FLAGS} -I${CMAKE_SOURCE_DIR}/test -I${CMAKE_SOURCE_DIR})
compile_proto(PROTO_HDRS PROTO_SRCS ${CMAKE_BINARY_DIR}/test
        ${CMAKE_BINARY_DIR}/test/hdrs
        ${CMAKE_SOURCE_DIR}/test
        "${TEST_PROTO_FILES}")

find_package(Gperftools)
include_directories(${GPERFTOOLS_INCLUDE_DIR})

include(require_gtest)
include(require_gmock)

set(GTM_LIB
        ${GTEST_LIB}
        ${GMOCK_LIB}
        )

add_definitions(
        -Dprivate=public
        -Dprotected=public
)

file(GLOB_RECURSE TESTING_SRC "${PROJECT_SOURCE_DIR}/test/testing/*.cc")

carbin_cc_library(
        NAMESPACE turbo
        NAME testing
        SOURCES ${TESTING_SRC} ${PROTO_SRCS}
        PRIVATE_COMPILE_OPTIONS ${CARBIN_DEFAULT_COPTS}
)


add_subdirectory(base)
add_subdirectory(bootstrap)
add_subdirectory(container)
add_subdirectory(fiber)
add_subdirectory(files)
add_subdirectory(hash)
add_subdirectory(io)
add_subdirectory(log)
add_subdirectory(memory)
add_subdirectory(metrics)
add_subdirectory(strings)
add_subdirectory(debugging)
add_subdirectory(times)
add_subdirectory(thread)
add_subdirectory(rapidjson)

