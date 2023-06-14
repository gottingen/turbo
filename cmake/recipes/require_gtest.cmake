
find_path(GTEST_INCLUDE_PATH NAMES gtest/gtest.h)
find_library(GTEST_LIB NAMES libgtest.a gtest)
find_library(GTEST_MAIN_LIB NAMES libgtest_main.a gtest_main)
if ((NOT GTEST_INCLUDE_PATH) OR (NOT GTEST_LIB) OR (NOT GTEST_MAIN_LIB))
    message(FATAL_ERROR "Fail to find gtest")
endif()
include_directories(${GTEST_INCLUDE_PATH})

