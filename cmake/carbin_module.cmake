
include(carbin_print)
include(carbin_cc_library)
include(carbin_cc_test)
include(carbin_cc_binary)
include(carbin_cc_benchmark)
include(carbin_print_list)
include(carbin_check)
include(carbin_variable)
include(carbin_include)


carbin_include("${PROJECT_BINARY_DIR}")
carbin_include("${PROJECT_SOURCE_DIR}")
carbin_include("${CMAKE_CURRENT_BINARY_DIR}")

MACRO(directory_list result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${curdir}/${child} )
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

include(carbin_outof_source)
include(carbin_platform)
include(carbin_pkg_dump)

CARBIN_ENSURE_OUT_OF_SOURCE_BUILD("must out of source dir")

#if (NOT DEV_MODE AND ${PROJECT_VERSION} MATCHES "0.0.0")
#    carbin_error("PROJECT_VERSION must be set in file project_profile or set -DDEV_MODE=true for develop debug")
#endif()


set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

