#
# Copyright 2024 The Carbin Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

list(APPEND CARBIN_CLANG_CL_FLAGS
        "/W3"
        "/DNOMINMAX"
        "/DWIN32_LEAN_AND_MEAN"
        "/D_CRT_SECURE_NO_WARNINGS"
        "/D_SCL_SECURE_NO_WARNINGS"
        "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
)

list(APPEND CARBIN_CLANG_CL_TEST_FLAGS
        "-Wno-c99-extensions"
        "-Wno-deprecated-declarations"
        "-Wno-missing-noreturn"
        "-Wno-missing-prototypes"
        "-Wno-missing-variable-declarations"
        "-Wno-null-conversion"
        "-Wno-shadow"
        "-Wno-shift-sign-overflow"
        "-Wno-sign-compare"
        "-Wno-unused-function"
        "-Wno-unused-member-function"
        "-Wno-unused-parameter"
        "-Wno-unused-private-field"
        "-Wno-unused-template"
        "-Wno-used-but-marked-unused"
        "-Wno-zero-as-null-pointer-constant"
        "-Wno-gnu-zero-variadic-macro-arguments"
)

list(APPEND CARBIN_GCC_FLAGS
        "-Wall"
        "-Wextra"
        "-Wno-cast-qual"
        "-Wconversion-null"
        "-Wformat-security"
        "-Woverlength-strings"
        "-Wpointer-arith"
        "-Wno-undef"
        "-Wunused-local-typedefs"
        "-Wunused-result"
        "-Wvarargs"
        "-Wno-attributes"
        "-Wno-implicit-fallthrough"
        "-Wno-unused-parameter"
        "-Wno-unused-function"
        "-Wwrite-strings"
        "-Wclass-memaccess"
        "-Wno-sign-compare"
        "-DNOMINMAX"
)

list(APPEND CARBIN_GCC_TEST_FLAGS
        "-Wno-conversion-null"
        "-Wno-deprecated-declarations"
        "-Wno-missing-declarations"
        "-Wno-sign-compare"
        "-Wno-undef"
        "-Wno-sign-compare"
        "-Wno-unused-function"
        "-Wno-unused-parameter"
        "-Wno-unused-private-field"
)

list(APPEND CARBIN_LLVM_FLAGS
        "-Wall"
        "-Wextra"
        "-Wno-cast-qual"
        "-Wno-conversion"
        "-Wno-sign-compare"
        "-Wfloat-overflow-conversion"
        "-Wfloat-zero-conversion"
        "-Wfor-loop-analysis"
        "-Wformat-security"
        "-Wgnu-redeclared-enum"
        "-Winfinite-recursion"
        "-Wliteral-conversion"
        "-Wmissing-declarations"
        "-Woverlength-strings"
        "-Wpointer-arith"
        "-Wself-assign"
        "-Wno-shadow"
        "-Wstring-conversion"
        "-Wtautological-overlap-compare"
        "-Wno-undef"
        "-Wuninitialized"
        "-Wunreachable-code"
        "-Wunused-comparison"
        "-Wunused-local-typedefs"
        "-Wunused-result"
        "-Wno-vla"
        "-Wwrite-strings"
        "-Wno-float-conversion"
        "-Wno-implicit-float-conversion"
        "-Wno-implicit-int-float-conversion"
        "-Wno-implicit-int-conversion"
        "-Wno-shorten-64-to-32"
        "-Wno-sign-conversion"
        "-Wno-unused-parameter"
        "-Wno-unused-function"
        "-DNOMINMAX"
)

list(APPEND CARBIN_LLVM_TEST_FLAGS
        "-Wno-c99-extensions"
        "-Wno-deprecated-declarations"
        "-Wno-missing-noreturn"
        "-Wno-missing-prototypes"
        "-Wno-missing-variable-declarations"
        "-Wno-null-conversion"
        "-Wno-shadow"
        "-Wno-undef"
        "-Wno-shift-sign-overflow"
        "-Wno-sign-compare"
        "-Wno-unused-function"
        "-Wno-unused-member-function"
        "-Wno-unused-parameter"
        "-Wno-unused-private-field"
        "-Wno-unused-template"
        "-Wno-sign-compare"
        "-Wno-unused-function"
        "-Wno-used-but-marked-unused"
        "-Wno-zero-as-null-pointer-constant"
        "-Wno-gnu-zero-variadic-macro-arguments"
)

list(APPEND CARBIN_MSVC_FLAGS
        "/W3"
        "/DNOMINMAX"
        "/DWIN32_LEAN_AND_MEAN"
        "/D_CRT_SECURE_NO_WARNINGS"
        "/D_SCL_SECURE_NO_WARNINGS"
        "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
        "/bigobj"
        "/wd4005"
        "/wd4068"
        "/wd4180"
        "/wd4244"
        "/wd4267"
        "/wd4503"
        "/wd4800"
)

list(APPEND CARBIN_MSVC_LINKOPTS
        "-ignore:4221"
)

list(APPEND CARBIN_MSVC_TEST_FLAGS
        "/wd4018"
        "/wd4101"
        "/wd4503"
        "/wd4996"
        "/DNOMINMAX"
)

list(APPEND CARBIN_RANDOM_HWAES_ARM32_FLAGS
        "-mfpu=neon"
)

list(APPEND CARBIN_RANDOM_HWAES_ARM64_FLAGS
        "-march=armv8-a+crypto"
)

list(APPEND CARBIN_RANDOM_HWAES_MSVC_X64_FLAGS
)

list(APPEND CARBIN_RANDOM_HWAES_X64_FLAGS
        "-maes"
        "-msse4.1"
)

################################################################################################
# cxx options
################################################################################################

set(CARBIN_LSAN_LINKOPTS "")
set(CARBIN_HAVE_LSAN OFF)
set(CARBIN_DEFAULT_LINKOPTS "")

if (BUILD_SHARED_LIBS AND MSVC)
    set(CARBIN_BUILD_DLL TRUE)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    set(CARBIN_BUILD_DLL FALSE)
endif()

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
    if (MSVC)
        set(CARBIN_RANDOM_RANDEN_COPTS "${CARBIN_RANDOM_HWAES_MSVC_X64_FLAGS}")
    else()
        set(CARBIN_RANDOM_RANDEN_COPTS "${CARBIN_RANDOM_HWAES_X64_FLAGS}")
    endif()
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm.*|aarch64")
    if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(CARBIN_RANDOM_RANDEN_COPTS "${CARBIN_RANDOM_HWAES_ARM64_FLAGS}")
    elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(CARBIN_RANDOM_RANDEN_COPTS "${CARBIN_RANDOM_HWAES_ARM32_FLAGS}")
    else()
        message(WARNING "Value of CMAKE_SIZEOF_VOID_P (${CMAKE_SIZEOF_VOID_P}) is not supported.")
    endif()
else()
    message(WARNING "Value of CMAKE_SYSTEM_PROCESSOR (${CMAKE_SYSTEM_PROCESSOR}) is unknown and cannot be used to set CARBIN_RANDOM_RANDEN_COPTS")
    set(CARBIN_RANDOM_RANDEN_COPTS "")
endif()


if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CARBIN_DEFAULT_COPTS "${CARBIN_GCC_FLAGS}")
    set(CARBIN_TEST_COPTS "${CARBIN_GCC_FLAGS};${CARBIN_GCC_TEST_FLAGS}")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # MATCHES so we get both Clang and AppleClang
    if(MSVC)
        # clang-cl is half MSVC, half LLVM
        set(CARBIN_DEFAULT_COPTS "${CARBIN_CLANG_CL_FLAGS}")
        set(CARBIN_TEST_COPTS "${CARBIN_CLANG_CL_FLAGS};${CARBIN_CLANG_CL_TEST_FLAGS}")
        set(CARBIN_DEFAULT_LINKOPTS "${CARBIN_MSVC_LINKOPTS}")
    else()
        set(CARBIN_DEFAULT_COPTS "${CARBIN_LLVM_FLAGS}")
        set(CARBIN_TEST_COPTS "${CARBIN_LLVM_FLAGS};${CARBIN_LLVM_TEST_FLAGS}")
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            # AppleClang doesn't have lsan
            # https://developer.apple.com/documentation/code_diagnostics
            if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
                set(CARBIN_LSAN_LINKOPTS "-fsanitize=leak")
                set(CARBIN_HAVE_LSAN ON)
            endif()
        endif()
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CARBIN_DEFAULT_COPTS "${CARBIN_MSVC_FLAGS}")
    set(CARBIN_TEST_COPTS "${CARBIN_MSVC_FLAGS};${CARBIN_MSVC_TEST_FLAGS}")
    set(CARBIN_DEFAULT_LINKOPTS "${CARBIN_MSVC_LINKOPTS}")
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER}.  Building with no default flags")
    set(CARBIN_DEFAULT_COPTS "")
    set(CARBIN_TEST_COPTS "")
endif()

##############################################################################
# default arch option
##############################################################################
set(CARBIN_ARCH_OPTION "")
if (CARBIN_ENABLE_ARCH)
    if (CXX_AVX2_FOUND)
        message(STATUS "AVX2 SUPPORTED for CXX")
        set(AVX2_SUPPORTED true)
        set(HIGHEST_SIMD_SUPPORTED "AVX2")
        list(APPEND SSE_SUPPORTED_LIST ${AVX2_SUPPORTED})
    else ()
        set(AVX2_SUPPORTED false)
    endif ()

    if (CXX_AVX512_FOUND)
        message(STATUS "AVX512 SUPPORTED for C and CXX")
        set(AVX512_SUPPORTED true)
        set(HIGHEST_SIMD_SUPPORTED "AVX512")
        list(APPEND SSE_SUPPORTED_LIST ${AVX512_SUPPORTED})
    else ()
        set(AVX512_SUPPORTED false)
    endif ()

    set(CARBIN_ARCH_OPTION)

    if (TURBO_USE_SSE1)
        message(STATUS "CARBIN SSE1 SELECTED")
        list(APPEND TURBO_SSE1_SIMD_FLAGS "-msse")
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_SSE1_SIMD_FLAGS})
    endif ()

    if (CXX_SSE2_FOUND)
        message(STATUS "CARBIN SSE2 SELECTED")
        list(APPEND TURBO_SSE2_SIMD_FLAGS "-msse2")
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_SSE2_SIMD_FLAGS})
    endif ()

    if (CXX_SSE3_FOUND)
        message(STATUS "CARBIN SSE3 SELECTED")
        list(APPEND TURBO_SSE3_SIMD_FLAGS "-msse3")
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_SSE3_SIMD_FLAGS})
    endif ()

    if (CXX_SSSE3_FOUND)
        message(STATUS "CARBIN SSSE3 SELECTED")
        list(APPEND TURBO_SSSE3_SIMD_FLAGS "-mssse3")
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_SSSE3_SIMD_FLAGS})
    endif ()

    if (CXX_SSE4_1_FOUND)
        message(STATUS "CARBIN SSE4_1 SELECTED")
        list(APPEND TURBO_SSE4_1_SIMD_FLAGS "-msse4.1")
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_SSE4_1_SIMD_FLAGS})
    endif ()

    if (CXX_SSE4_2_FOUND)
        message(STATUS "CARBIN SSE4_2 SELECTED")
        list(APPEND TURBO_SSE4_2_SIMD_FLAGS "-msse4.2")
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_SSE4_2_SIMD_FLAGS})
    endif ()

    if (CXX_AVX_FOUND)
        message(STATUS "CARBIN AVX SELECTED")
        list(APPEND TURBO_AVX_SIMD_FLAGS "-mavx")
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_AVX_SIMD_FLAGS})
    endif ()

    if (CXX_AVX2_FOUND)
        message(STATUS "CARBIN AVX2 SELECTED")
        list(APPEND TURBO_AVX2_SIMD_FLAGS "-mavx2" "-mfma")
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_AVX2_SIMD_FLAGS})
    endif ()

    if (CXX_AVX512_FOUND)
        message(STATUS "CARBIN AVX512 SELECTED")
        list(APPEND TURBO_AVX512_SIMD_FLAGS "-mavx512f" "-mfma") # note that this is a bit platform specific
        list(APPEND CARBIN_ARCH_OPTION ${TURBO_AVX512_SIMD_FLAGS}) # note that this is a bit platform specific
    endif ()
endif ()
list(APPEND CARBIN_ARCH_OPTION ${CARBIN_RANDOM_RANDEN_COPTS})
MESSAGE(STATUS "CARBIN ARCH FLAGS ${CARBIN_ARCH_OPTION}")

##############################################################
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-g -O2")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if(DEFINED ENV{CARBIN_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{CARBIN_CXX_FLAGS}")
endif ()

################################
# follow CC flag we provide
# ${CARBIN_DEFAULT_COPTS}
# ${CARBIN_TEST_COPTS}
# ${CARBIN_ARCH_OPTION} for arch option, by default, we set enable and
# ${CARBIN_RANDOM_RANDEN_COPTS}
# set it to haswell arch
##############################################################################
set(CARBIN_CXX_OPTIONS ${CARBIN_DEFAULT_COPTS} ${CARBIN_ARCH_OPTION} ${CARBIN_RANDOM_RANDEN_COPTS})
###############################
#
# define you options here
# eg.
# list(APPEND CARBIN_CXX_OPTIONS "-fopenmp")
list(REMOVE_DUPLICATES CARBIN_CXX_OPTIONS)
carbin_print_list_label("CXX_OPTIONS:" CARBIN_CXX_OPTIONS)