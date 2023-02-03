
file(GLOB SRC "*.cc")

foreach (fl ${SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" TEST_NAME ${fl})
    get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    string(REPLACE " " "_" DIR_NAME ${DIR_NAME})

    set(EXE_NAME ${DIR_NAME}_${TEST_NAME})
    turbo_cc_test(
            NAME ${EXE_NAME}
            SRCS ${fl}
            LINKOPTS ${CARBIN_DYLINK} ${DYNAMIC_LIB}
            COPTS ${CARBIN_TEST_COPTS}
    )
endforeach (fl ${SRC})