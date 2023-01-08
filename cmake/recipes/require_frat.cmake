
find_path(FRAT_INCLUDE_PATH NAMES frat/log.h)
find_library(FRAT_LIB NAMES frat)
include_directories(${FRAT_INCLUDE_PATH})
if((NOT FRAT_INCLUDE_PATH) OR (NOT FRAT_LIB))
    message(FATAL_ERROR "Fail to find frat")
endif()
