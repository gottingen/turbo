
find_package (PkgConfig REQUIRED)

pkg_check_modules (c-ares_PC
        QUIET
        libcares)

find_library (c-ares_LIBRARY
        NAMES cares
        HINTS
        ${c-ares_PC_LIBDIR}
        ${c-ares_PC_LIBRARY_DIRS})

find_path (c-ares_INCLUDE_DIR
        NAMES ares_dns.h
        HINTS
        ${c-ares_PC_INCLUDEDIR}
        ${c-ares_PC_INCLUDE_DIRS})

mark_as_advanced (
        c-ares_LIBRARY
        c-ares_INCLUDE_DIR)

include (FindPackageHandleStandardArgs)

find_package_handle_standard_args (c-ares
        REQUIRED_VARS
        c-ares_LIBRARY
        c-ares_INCLUDE_DIR
        VERSION_VAR c-ares_PC_VERSION)

set (C-ARES_LIBRARIES ${c-ares_LIBRARY})
set (C-ARES_INCLUDE_DIRS ${c-ares_INCLUDE_DIR})

if (c-ares_FOUND AND NOT (TARGET c-ares::c-ares))
    add_library (c-ares::c-ares UNKNOWN IMPORTED)

    set_target_properties (c-ares::c-ares
            PROPERTIES
            IMPORTED_LOCATION ${c-ares_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${C-ARES_INCLUDE_DIRS})
endif ()
