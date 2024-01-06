platform
===

# What is TBBase?

TBBase is a small set of header files that define 
platform-independent data types and macros. As such 
it is similar to many projects that have a platform.h,
system.h, defines.h, etc. file. The difference is that 
TBBase is very comprehensive and is the annointed 
Electronic Arts worldwide standard for new projects.

With respect to the base types and definitions, many 
of these are already present in the most recent C 
language standard, though the C++ standard has yet to 
formally adopt them. TBBase bridges the gap and defines
these values if they aren't already defined. With 
respect to compiler and platform definitions, TBBase 
provides a standard reliable means of identifying or 
specifying compilers, platforms, endian-ness, alignment 
attributes, etc.

# Usage notes

You probably don't want to use float_t and double_t. 
They are there for C99 compatibility but are rarely 
what you want to use, since their size is variable.

Prid8, etc. are somewhat painful and ugly to use and 
you may find you don't like them. They too are for 
C99 compatibility.

intptr_t is not a pointer to an int; it's an int with 
the same size as a pointer, so you can safely store 
pointers in it.

turbo::result_type is rarely used and exists for backwards 
compatibility.

# What specifically does TBBase define?

Here we list the things TBBase defines, grouped by 
category. These defines are up to date as of the 
file modification date listed at the top of this file.

## Base Types and Definitions

bool8_t, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float_t, double_t, (EAStdC package 
implements int128_t)

intptr_t, uintptr_t, intmax_t, uintmax_t, ssize_t

char8_t, char16_t, char32_t

INT8_C(), UINT8_C(), etc.

INT8_MIN, INT8_MAX, UINT8_MAX, etc.

PRId8, PRId16, PRId32, etc, SCNd8, SCNd16, SCNd32, etc.

## Result Types and Definitions

turbo::result_type

turbo::SUCCESS, turbo::FAILURE

TURBO_SUCCEEDED(), TURBO_FAILED()

## Compiler Definitions

TURBO_COMPILER_GNUC
TURBO_COMPILER_SN
TURBO_COMPILER_MSVC
TURBO_COMPILER_METROWERKS
TURBO_COMPILER_INTEL
TURBO_COMPILER_BORLANDC

TURBO_COMPILER_VERSION = <integer>
TURBO_COMPILER_NAME = <string>
TURBO_COMPILER_STRING = <string>

TURBO_COMPILER_NO_STATIC_CONSTANTS
TURBO_COMPILER_NO_TEMPLATE_SPECIALIZATION
TURBO_COMPILER_NO_TEMPLATE_PARTIAL_SPECIALIZATION
TURBO_COMPILER_NO_MEMBER_TEMPLATES
TURBO_COMPILER_NO_MEMBER_TEMPLATE_SPECIALIZATION
TURBO_COMPILER_NO_TEMPLATE_TEMPLATES
TURBO_COMPILER_NO_MEMBER_TEMPLATE_FRIENDS
TURBO_COMPILER_NO_VOID_RETURNS
TURBO_COMPILER_NO_COVARIANT_RETURN_TYPE
TURBO_COMPILER_NO_DEDUCED_TYPENAME
TURBO_COMPILER_NO_ARGUMENT_DEPENDENT_LOOKUP
TURBO_COMPILER_NO_EXCEPTION_STD_NAMESPACE
TURBO_COMPILER_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS
TURBO_COMPILER_NO_EXCEPTIONS
TURBO_COMPILER_NO_UNWIND

TURBO_COMPILER_IS_ANSIC
TURBO_COMPILER_IS_C99
TURBO_COMPILER_HAS_C99_TYPES
TURBO_COMPILER_IS_CPLUSPLUS
TURBO_COMPILER_MANAGED_CPP

## Utilities

TURBO_ALIGN_OF()
TURBO_PREFIX_ALIGN()
TURBO_POSTFIX_ALIGN()
TURBO_ALIGNED()
TURBO_PACKED()
TURBO_LIKELY()
TURBO_UNLIKELY()
TURBO_ASSUME()
TURBO_PURE
TURBO_WCHAR_T_NON_NATIVE
TURBO_WCHAR_SIZE
TURBO_RESTRICT
TURBO_PREFIX_DEPRECATED
TURBO_POSTFIX_DEPRECATED
TURBO_FORCE_INLINE
TURBO_NO_INLINE
TURBO_PREFIX_NO_INLINE
TURBO_POSTFIX_NO_INLINE
TURBO_PASCAL
TURBO_PASCAL_FUNC()
TURBO_IMPORT
TURBO_EXPORT
TURBO_API
TURBO_OVERRIDE
TURBO_INIT_PRIORITY
TURBO_MAY_ALIAS

## Platform Definitions

TURBO_PLATFORM_MAC
TURBO_PLATFORM_OSX
TURBO_PLATFORM_IPHONE
TURBO_PLATFORM_ANDROID
TURBO_PLATFORM_LINUX
TURBO_PLATFORM_WINDOWS
TURBO_PLATFORM_WIN32
TURBO_PLATFORM_WIN64
TURBO_PLATFORM_HPUX
TURBO_PLATFORM_SUN

TURBO_PLATFORM_NAME
TURBO_PLATFORM_DESCRIPTION
TURBO_PROCESSOR_POWERPC, TURBO_PROCESSOR_X86, TURBO_PROCESSOR_ARM, etc.
TURBO_SYSTEM_LITTLE_ENDIAN, TURBO_SYSTEM_BIG_ENDIAN
TURBO_ASM_STYLE_ATT, TURBO_ASM_STYLE_INTEL, TURBO_ASM_STYLE_MOTOROLA
TURBO_PLATFORM_PTR_SIZE
TURBO_PLATFORM_WORD_SIZE

# system defines

TURBO_GLIBC_VERSION
