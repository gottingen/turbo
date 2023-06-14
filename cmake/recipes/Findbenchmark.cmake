# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# Findbenchmark
# ---------
#
# Locate the Google C++ Testing Framework.
#
# Imported targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` targets:
#
# ``GTest::GTest``
#   The Google Test ``benchmark`` library, if found; adds Thread::Thread
#   automatically
# ``benchmark::benchmark_main``
#   The Google Test ``benchmark_main`` library, if found
#
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``BENCHMARK_FOUND``
#   Found the Google Testing framework
# ``BENCHMARK_INCLUDE_DIRS``
#   the directory containing the Google Test headers
#
# The library variables below are set as normal variables.  These
# contain debug/optimized keywords when a debugging library is found.
#
# ``BENCHMARK_LIBRARIES``
#   The Google Test ``benchmark`` library; note it also requires linking
#   with an appropriate thread library
# ``BENCHMARK_MAIN_LIBRARIES``
#   The Google Test ``benchmark_main`` library
# ``BENCHMARK_BOTH_LIBRARIES``
#   Both ``benchmark`` and ``benchmark_main``
#
# Cache variables
# ^^^^^^^^^^^^^^^
#
# The following cache variables may also be set:
#
# ``BENCHMARK_ROOT``
#   The root directory of the Google Test installation (may also be
#   set as an environment variable)
# ``BENCHMARK_MSVC_SEARCH``
#   If compiling with MSVC, this variable can be set to ``MT`` or
#   ``MD`` (the default) to enable searching a GTest build tree
#
#
# Example usage
# ^^^^^^^^^^^^^
#
# ::
#
#     enable_testing()
#     find_package(GTest REQUIRED)
#
#     add_executable(foo foo.cc)
#     target_link_libraries(foo GTest::GTest benchmark::benchmark_main)
#
#     add_test(AllTestsInFoo foo)
#
#
# Deeper integration with CTest
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#
# See :module:`GoogleTest` for information on the :command:`benchmark_add_tests`
# command.

function(_benchmark_append_debugs _endvar _library)
    if (${_library} AND ${_library}_DEBUG)
        set(_output optimized ${${_library}} debug ${${_library}_DEBUG})
    else ()
        set(_output ${${_library}})
    endif ()
    set(${_endvar} ${_output} PARENT_SCOPE)
endfunction()

function(_benchmark_find_library _name)
    find_library(${_name}
            NAMES ${ARGN}
            HINTS
            ENV BENCHMARK_ROOT
            ${BENCHMARK_ROOT}
            PATH_SUFFIXES ${_benchmark_libpath_suffixes}
            )
    mark_as_advanced(${_name})
endfunction()

#

if (NOT DEFINED BENCHMARK_MSVC_SEARCH)
    set(BENCHMARK_MSVC_SEARCH MD)
endif ()

set(_benchmark_libpath_suffixes lib)
if (MSVC)
    if (BENCHMARK_MSVC_SEARCH STREQUAL "MD")
        list(APPEND _benchmark_libpath_suffixes
                msvc/benchmark-md/Debug
                msvc/benchmark-md/Release
                msvc/x64/Debug
                msvc/x64/Release
                )
    elseif (BENCHMARK_MSVC_SEARCH STREQUAL "MT")
        list(APPEND _benchmark_libpath_suffixes
                msvc/benchmark/Debug
                msvc/benchmark/Release
                msvc/x64/Debug
                msvc/x64/Release
                )
    endif ()
endif ()


find_path(BENCHMARK_INCLUDE_DIR benchmark/benchmark.h
        HINTS
        $ENV{BENCHMARK_ROOT}/include
        ${BENCHMARK_ROOT}/include
        )
mark_as_advanced(BENCHMARK_INCLUDE_DIR)

if (MSVC AND BENCHMARK_MSVC_SEARCH STREQUAL "MD")
    # The provided /MD project files for Google Test add -md suffixes to the
    # library names.
    _benchmark_find_library(BENCHMARK_LIBRARY benchmark-md benchmark)
    _benchmark_find_library(BENCHMARK_LIBRARY_DEBUG benchmark-mdd benchmarkd)
    _benchmark_find_library(BENCHMARK_MAIN_LIBRARY benchmark_main-md benchmark_main)
    _benchmark_find_library(BENCHMARK_MAIN_LIBRARY_DEBUG benchmark_main-mdd benchmark_maind)
else ()
    _benchmark_find_library(BENCHMARK_LIBRARY benchmark)
    _benchmark_find_library(BENCHMARK_LIBRARY_DEBUG benchmarkd)
    _benchmark_find_library(BENCHMARK_MAIN_LIBRARY benchmark_main)
    _benchmark_find_library(BENCHMARK_MAIN_LIBRARY_DEBUG benchmark_maind)
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Benchmark DEFAULT_MSG BENCHMARK_LIBRARY BENCHMARK_INCLUDE_DIR BENCHMARK_MAIN_LIBRARY)

if (BENCHMARK_FOUND)
    set(BENCHMARK_INCLUDE_DIRS ${BENCHMARK_INCLUDE_DIR})
    _benchmark_append_debugs(BENCHMARK_LIBRARIES BENCHMARK_LIBRARY)
    _benchmark_append_debugs(BENCHMARK_MAIN_LIBRARIES BENCHMARK_MAIN_LIBRARY)
    set(BENCHMARK_BOTH_LIBRARIES ${BENCHMARK_LIBRARIES} ${BENCHMARK_MAIN_LIBRARIES})

    include(CMakeFindDependencyMacro)
    find_dependency(Threads)

    if (NOT TARGET benchmark::benchmark)
        add_library(benchmark::benchmark UNKNOWN IMPORTED)
        set_target_properties(benchmark::benchmark PROPERTIES
                INTERFACE_LINK_LIBRARIES "Threads::Threads")
        if (BENCHMARK_INCLUDE_DIRS)
            set_target_properties(benchmark::benchmark PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${BENCHMARK_INCLUDE_DIRS}")
        endif ()
        if (EXISTS "${BENCHMARK_LIBRARY}")
            set_target_properties(benchmark::benchmark PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
                    IMPORTED_LOCATION "${BENCHMARK_LIBRARY}")
        endif ()
        if (EXISTS "${BENCHMARK_LIBRARY_RELEASE}")
            set_property(TARGET benchmark::benchmark APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(benchmark::benchmark PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
                    IMPORTED_LOCATION_RELEASE "${BENCHMARK_LIBRARY_RELEASE}")
        endif ()
        if (EXISTS "${BENCHMARK_LIBRARY_DEBUG}")
            set_property(TARGET benchmark::benchmark APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(benchmark::benchmark PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
                    IMPORTED_LOCATION_DEBUG "${BENCHMARK_LIBRARY_DEBUG}")
        endif ()
    endif ()
    if (NOT TARGET benchmark::benchmark_main)
        add_library(benchmark::benchmark_main UNKNOWN IMPORTED)
        set_target_properties(benchmark::benchmark_main PROPERTIES
                INTERFACE_LINK_LIBRARIES "benchmark::benchmark_main")
        if (EXISTS "${BENCHMARK_MAIN_LIBRARY}")
            set_target_properties(benchmark::benchmark_main PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
                    IMPORTED_LOCATION "${BENCHMARK_MAIN_LIBRARY}")
        endif ()
        if (EXISTS "${BENCHMARK_MAIN_LIBRARY_RELEASE}")
            set_property(TARGET benchmark::benchmark_main APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(benchmark::benchmark_main PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
                    IMPORTED_LOCATION_RELEASE "${BENCHMARK_MAIN_LIBRARY_RELEASE}")
        endif ()
        if (EXISTS "${BENCHMARK_MAIN_LIBRARY_DEBUG}")
            set_property(TARGET benchmark::benchmark_main APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(benchmark::benchmark_main PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
                    IMPORTED_LOCATION_DEBUG "${BENCHMARK_MAIN_LIBRARY_DEBUG}")
        endif ()
    endif ()
endif ()
