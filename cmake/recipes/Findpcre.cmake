
include(FindPackageHandleStandardArgs)
find_path(PCRE_INCLUDE_DIR NAMES pcre.h)
find_library(PCRE_LIBRARY NAMES pcre)
find_package_handle_standard_args(
        PCRE
        DEFAULT_MSG
        PCRE_LIBRARY
        PCRE_INCLUDE_DIR
)
mark_as_advanced(PCRE_INCLUDE_DIR PCRE_LIBRARY)

if (PCRE_FOUND)
    set(PCRE_LIBRARIES ${PCRE_LIBRARY})
    set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})

    if (NOT TARGET pcre::pcre)
        add_library(pcre::pcre UNKNOWN IMPORTED)
        set_target_properties(pcre::pcre PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIRS}")
        set_target_properties(pcre::pcre PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${PCRE_LIBRARIES}")
    endif ()
endif ()