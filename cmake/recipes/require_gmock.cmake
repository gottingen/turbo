
find_path(GMOCK_INCLUDE_PATH NAMES gmock/gmock.h)
find_library(GMOCK_LIB NAMES libgmock.a gmock)
find_library(GMOCK_MAIN_LIB NAMES libgmock_main.a gmock_main)
if ((NOT GMOCK_INCLUDE_PATH) OR (NOT GMOCK_LIB) OR (NOT GMOCK_MAIN_LIB))
    message(FATAL_ERROR "Fail to find gmock")
endif()
include_directories(${GMOCK_INCLUDE_PATH})