# Find the Snappy libraries
#
# This module defines:
# SNAPPY_FOUND
# SNAPPY_INCLUDE_DIRS
# SNAPPY_LIBRARIES

find_path(SNAPPY_INCLUDE_DIR NAMES snappy.h)

find_library(SNAPPY_LIBRARY_DEBUG NAMES snappyd)
find_library(SNAPPY_LIBRARY_RELEASE NAMES snappy)

include(SelectLibraryConfigurations)
SELECT_LIBRARY_CONFIGURATIONS(SNAPPY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
        SNAPPY DEFAULT_MSG
        SNAPPY_LIBRARY SNAPPY_INCLUDE_DIR
)

mark_as_advanced(SNAPPY_INCLUDE_DIR SNAPPY_LIBRARY)

if(SNAPPY_FOUND)
    set(SNAPPY_LIBRARIES ${SNAPPY_LIBRARY})
    set(SNAPPY_INCLUDE_DIRS ${SNAPPY_INCLUDE_DIR})

    if (NOT TARGET snappy::snappy)
        add_library(snappy::snappy UNKNOWN IMPORTED)
        set_target_properties(snappy::snappy PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SNAPPY_INCLUDE_DIRS}")
        set_target_properties(snappy::snappy PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "CXX" IMPORTED_LOCATION "${SNAPPY_LIBRARIES}")
    endif ()

endif()