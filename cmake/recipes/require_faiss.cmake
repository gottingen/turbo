

find_path(FAISS_INCLUDE_PATH NAMES faiss/Index.h)
find_library(FAISS_LIB NAMES faiss)
if ((NOT FAISS_INCLUDE_PATH) OR (NOT FAISS_LIB))
    message(FATAL_ERROR "Fail to find faiss")
endif()
include_directories(${FAISS_INCLUDE_PATH})