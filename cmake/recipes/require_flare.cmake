
find_path(FLARE_INCLUDE_PATH NAMES flare/idl_options.pb.h)
find_library(FLARE_LIB NAMES flare)
include_directories(${FLARE_INCLUDE_PATH})
if((NOT FLARE_INCLUDE_PATH) OR (NOT FLARE_LIB))
    message(FATAL_ERROR "Fail to find flare")
endif()
