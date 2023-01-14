
find_path(TURBO_INCLUDE_PATH NAMES turbo/idl_options.pb.h)
find_library(TURBO_LIB NAMES turbo)
include_directories(${TURBO_INCLUDE_PATH})
if((NOT TURBO_INCLUDE_PATH) OR (NOT TURBO_LIB))
    message(FATAL_ERROR "Fail to find turbo")
endif()
