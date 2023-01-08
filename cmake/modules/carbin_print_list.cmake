

include(carbin_print)
include(carbin_color)
MACRO(carbin_directory_list result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${curdir}/${child} )
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

MACRO(carbin_print_list result)
    foreach(arg IN LISTS ${result})
        message(" - ${carbin_cyan}${arg}${carbin_colour_reset}")
    endforeach()
ENDMACRO()


MACRO(carbin_print_list_label Label ListVar)
    message("${carbin_yellow}${Label}${carbin_colour_reset}:")
    carbin_print_list(${ListVar})
ENDMACRO()



