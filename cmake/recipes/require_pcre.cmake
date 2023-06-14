
find_path(PCRE_INCLUDE_PATH NAMES pcre.h)
find_library(PCRE_LIB NAMES pcre)
include_directories(${PCRE_INCLUDE_PATH})
if((NOT PCRE_INCLUDE_PATH) OR (NOT PCRE_LIB))
    message(FATAL_ERROR "Fail to find pcre")
endif()
