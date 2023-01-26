/*-----------------------------------------------------------------------------
 * eahave.h
 *
 * Copyright (c) Electronic Arts Inc. All rights reserved.
 * Copyright (c) Jeff.li. All rights reserved.
 *---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
	This file's functionality is preliminary and won't be considered stable until 
	a future TBBase version.
 *---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
	This header identifies if the given facilities are available in the 
	standard build environment the current compiler/linker/standard library/
	operating system combination. This file may in some cases #include standard
	headers in order to make availability determinations, such as to check 
	compiler or SDK version numbers. However, it cannot be perfect.
	This header does not identify compiler features, as those are defined in 
	eacompiler.h and eacompilertraits.h. Rather this header is about library support.
	This header does not identify platform or library conventions either, such
	as whether the file paths use \ or / for directory separators.

	We provide three types of HAVE features here:

		- TURBO_HAVE_XXX_FEATURE - Have compiler feature.
		  Identifies if the compiler has or lacks some feature in the 
		  current build. Sometimes you need to check to see if the 
		  compiler is running in some mode in able to write portable code
		  against it. For example, some compilers (e.g. VC++) have a 
		  mode in which all language extensions are disabled. If you want
		  to write code that works with that but still uses the extensions
		  when available then you can check #if defined(TURBO_HAVE_EXTENSIONS_FEATURE).
		  Features can be forcibly cancelled via TURBO_NO_HAVE_XXX_FEATURE.
		  TURBO_NO_HAVE is useful for a build system or user to override the
		  defaults because it happens to know better.

		- TURBO_HAVE_XXX_H - Have header file information.
		  Identifies if a given header file is available to the current 
		  compile configuration. For example, some compilers provide a 
		  malloc.h header, while others don't. For the former we define 
		  TURBO_HAVE_MALLOC_H, while for the latter it remains undefined.
		  If a header is missing then it may still be that the functions
		  the header usually declares are declared in some other header.
		  TURBO_HAVE_XXX does not include the possibility that our own code
		  provides versions of these headers, and in fact a purpose of 
		  TURBO_HAVE_XXX is to decide if we should be using our own because
		  the system doesn't provide one.
		  Header availability can be forcibly cancelled via TURBO_NO_HAVE_XXX_H.
		  TURBO_NO_HAVE is useful for a build system or user to override the
		  defaults because it happens to know better.

		- TURBO_HAVE_XXX_DECL - Have function declaration information.
		  Identifies if a given function declaration is provided by 
		  the current compile configuration. For example, some compiler
		  standard libraries declare a wcslen function, while others
		  don't. For the former we define TURBO_HAVE_WCSLEN_DECL, while for
		  the latter it remains undefined. If a declaration of a function
		  is missing then we assume the implementation is missing as well.
		  TURBO_HAVE_XXX_DECL does not include the possibility that our
		  own code provides versions of these declarations, and in fact a 
		  purpose of TURBO_HAVE_XXX_DECL is to decide if we should be using
		  our own because the system doesn't provide one.
		  Declaration availability can be forcibly cancelled via TURBO_NO_HAVE_XXX_DECL.
		  TURBO_NO_HAVE is useful for a build system or user to override the
		  defaults because it happens to know better.

		- TURBO_HAVE_XXX_IMPL - Have function implementation information.
		  Identifies if a given function implementation is provided by
		  the current compile and link configuration. For example, it's
		  commonly the case that console platforms declare a getenv function 
		  but don't provide a linkable implementation.
		  In this case the user needs to provide such a function manually
		  as part of the link. If the implementation is available then
		  we define TURBO_HAVE_GETENV_IMPL, otherwise it remains undefined.
		  Beware that sometimes a function may not seem to be present in 
		  the Standard Library but in reality you need to link some auxiliary
		  provided library for it. An example of this is the Unix real-time
		  functions such as clock_gettime.
		  TURBO_HAVE_XXX_IMPL does not include the possibility that our
		  own code provides versions of these implementations, and in fact a 
		  purpose of TURBO_HAVE_XXX_IMPL is to decide if we should be using
		  our own because the system doesn't provide one.
		  Implementation availability can be forcibly cancelled via TURBO_NO_HAVE_XXX_IMPL.
		  TURBO_NO_HAVE is useful for a build system or user to override the
		  defaults because it happens to know better.

	It's not practical to define TURBO_HAVE macros for every possible header,
	declaration, and implementation, and so the user must simply know that
	some headers, declarations, and implementations tend to require TURBO_HAVE
	checking. Nearly every C Standard Library we've seen has a <string.h> 
	header, a strlen declaration, and a linkable strlen implementation, 
	so there's no need to provide TURBO_HAVE support for this. On the other hand
	it's commonly the case that the C Standard Library doesn't have a malloc.h
	header or an inet_ntop declaration.

---------------------------------------------------------------------------*/


#ifndef TURBO_PLATFORM_CONFIG_HAVE_H_
#define TURBO_PLATFORM_CONFIG_HAVE_H_


#include "turbo/platform/config/base.h"


#if defined(TURBO_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

/* TURBO_HAVE_XXX_FEATURE */

#if !defined(TURBO_HAVE_EXTENSIONS_FEATURE) && !defined(TURBO_NO_HAVE_EXTENSIONS_FEATURE)
	#define TURBO_HAVE_EXTENSIONS_FEATURE 1
#endif


/* TURBO_HAVE_XXX_LIBRARY */

// Dinkumware
#if !defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && !defined(TURBO_NO_HAVE_DINKUMWARE_CPP_LIBRARY)
	#if defined(__cplusplus)
		TURBO_DISABLE_ALL_VC_WARNINGS()
		#include <cstddef> // Need to trigger the compilation of yvals.h without directly using <yvals.h> because it might not exist.
		TURBO_RESTORE_ALL_VC_WARNINGS()
	#endif

	#if defined(__cplusplus) && defined(_CPPLIB_VER) /* If using the Dinkumware Standard library... */
		#define TURBO_HAVE_DINKUMWARE_CPP_LIBRARY 1
	#else
		#define TURBO_NO_HAVE_DINKUMWARE_CPP_LIBRARY 1
	#endif
#endif

// GCC libstdc++
#if !defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && !defined(TURBO_NO_HAVE_LIBSTDCPP_LIBRARY)
	#if defined(__GLIBCXX__) /* If using libstdc++ ... */
		#define TURBO_HAVE_LIBSTDCPP_LIBRARY 1
	#else
		#define TURBO_NO_HAVE_LIBSTDCPP_LIBRARY 1
	#endif
#endif

// Clang libc++
#if !defined(TURBO_HAVE_LIBCPP_LIBRARY) && !defined(TURBO_NO_HAVE_LIBCPP_LIBRARY)
	#if TURBO_HAS_INCLUDE_AVAILABLE
		#if TURBO_HAS_INCLUDE(<__config>)
			#define TURBO_HAVE_LIBCPP_LIBRARY 1 // We could also #include <ciso646> and check if defined(_LIBCPP_VERSION).
		#endif
	#endif

	#if !defined(TURBO_HAVE_LIBCPP_LIBRARY)
		#define TURBO_NO_HAVE_LIBCPP_LIBRARY 1
	#endif
#endif


/* TURBO_HAVE_XXX_H */

// #include <sys/types.h>
#if !defined(TURBO_HAVE_SYS_TYPES_H) && !defined(TURBO_NO_HAVE_SYS_TYPES_H)
		#define TURBO_HAVE_SYS_TYPES_H 1
#endif

// #include <io.h> (and not sys/io.h or asm/io.h)
#if !defined(TURBO_HAVE_IO_H) && !defined(TURBO_NO_HAVE_IO_H)
	// Unix doesn't have Microsoft's <io.h> but has the same functionality in <fcntl.h> and <sys/stat.h>.
	#if defined(TURBO_PLATFORM_MICROSOFT)
		#define TURBO_HAVE_IO_H 1
	#else
		#define TURBO_NO_HAVE_IO_H 1
	#endif
#endif

// #include <inttypes.h>
#if !defined(TURBO_HAVE_INTTYPES_H) && !defined(TURBO_NO_HAVE_INTTYPES_H)
	#if !defined(TURBO_PLATFORM_MICROSOFT)
		#define TURBO_HAVE_INTTYPES_H 1
	#else
		#define TURBO_NO_HAVE_INTTYPES_H 1
	#endif
#endif

// #include <unistd.h>
#if !defined(TURBO_HAVE_UNISTD_H) && !defined(TURBO_NO_HAVE_UNISTD_H)
	#if defined(TURBO_PLATFORM_UNIX)
		#define TURBO_HAVE_UNISTD_H 1
	#else
		#define TURBO_NO_HAVE_UNISTD_H 1
	#endif
#endif

// #include <sys/time.h>
#if !defined(TURBO_HAVE_SYS_TIME_H) && !defined(TURBO_NO_HAVE_SYS_TIME_H)
	#if !defined(TURBO_PLATFORM_MICROSOFT) && !defined(_CPPLIB_VER) /* _CPPLIB_VER indicates Dinkumware. */
		#define TURBO_HAVE_SYS_TIME_H 1 /* defines struct timeval */
	#else
		#define TURBO_NO_HAVE_SYS_TIME_H 1
	#endif
#endif

// #include <ptrace.h>
#if !defined(TURBO_HAVE_SYS_PTRACE_H) && !defined(TURBO_NO_HAVE_SYS_PTRACE_H)
	#if defined(TURBO_PLATFORM_UNIX) && !defined(__CYGWIN__) && (defined(TURBO_PLATFORM_DESKTOP) || defined(TURBO_PLATFORM_SERVER))
		#define TURBO_HAVE_SYS_PTRACE_H 1 /* declares the ptrace function */
	#else
		#define TURBO_NO_HAVE_SYS_PTRACE_H 1
	#endif
#endif

// #include <sys/stat.h>
#if !defined(TURBO_HAVE_SYS_STAT_H) && !defined(TURBO_NO_HAVE_SYS_STAT_H)
	#if (defined(TURBO_PLATFORM_UNIX) && !(defined(TURBO_PLATFORM_SONY) && defined(TURBO_PLATFORM_CONSOLE))) || defined(__APPLE__) || defined(TURBO_PLATFORM_ANDROID)
		#define TURBO_HAVE_SYS_STAT_H 1 /* declares the stat struct and function */
	#else
		#define TURBO_NO_HAVE_SYS_STAT_H 1
	#endif
#endif

// #include <locale.h>
#if !defined(TURBO_HAVE_LOCALE_H) && !defined(TURBO_NO_HAVE_LOCALE_H)
		#define TURBO_HAVE_LOCALE_H 1
#endif

// #include <signal.h>
#if !defined(TURBO_HAVE_SIGNAL_H) && !defined(TURBO_NO_HAVE_SIGNAL_H)
	#if !defined(TURBO_PLATFORM_BSD) && !defined(TURBO_PLATFORM_SONY) && !defined(CS_UNDEFINED_STRING)
		#define TURBO_HAVE_SIGNAL_H 1
	#else
		#define TURBO_NO_HAVE_SIGNAL_H 1
	#endif
#endif

// #include <sys/signal.h>
#if !defined(TURBO_HAVE_SYS_SIGNAL_H) && !defined(TURBO_NO_HAVE_SYS_SIGNAL_H)
	#if defined(TURBO_PLATFORM_BSD) || defined(TURBO_PLATFORM_SONY)
		#define TURBO_HAVE_SYS_SIGNAL_H 1
	#else
		#define TURBO_NO_HAVE_SYS_SIGNAL_H 1
	#endif
#endif

// #include <pthread.h>
#if !defined(TURBO_HAVE_PTHREAD_H) && !defined(TURBO_NO_HAVE_PTHREAD_H)
	#if defined(TURBO_PLATFORM_UNIX) || defined(TURBO_PLATFORM_APPLE) || defined(TURBO_PLATFORM_POSIX)
		#define TURBO_HAVE_PTHREAD_H 1 /* It can be had under Microsoft/Windows with the http://sourceware.org/pthreads-win32/ library */
	#else
		#define TURBO_NO_HAVE_PTHREAD_H 1
	#endif
#endif

// #include <wchar.h>
#if !defined(TURBO_HAVE_WCHAR_H) && !defined(TURBO_NO_HAVE_WCHAR_H)
	#if defined(TURBO_PLATFORM_DESKTOP) && defined(TURBO_PLATFORM_UNIX) && defined(TURBO_PLATFORM_SONY) && defined(TURBO_PLATFORM_APPLE)
		#define TURBO_HAVE_WCHAR_H 1
	#else
		#define TURBO_NO_HAVE_WCHAR_H 1
	#endif
#endif

// #include <malloc.h>
#if !defined(TURBO_HAVE_MALLOC_H) && !defined(TURBO_NO_HAVE_MALLOC_H)
	#if defined(_MSC_VER) || defined(__MINGW32__)
		#define TURBO_HAVE_MALLOC_H 1
	#else
		#define TURBO_NO_HAVE_MALLOC_H 1
	#endif
#endif

// #include <alloca.h>
#if !defined(TURBO_HAVE_ALLOCA_H) && !defined(TURBO_NO_HAVE_ALLOCA_H)
	#if !defined(TURBO_HAVE_MALLOC_H) && !defined(TURBO_PLATFORM_SONY)
		#define TURBO_HAVE_ALLOCA_H 1
	#else
		#define TURBO_NO_HAVE_ALLOCA_H 1
	#endif
#endif

// #include <execinfo.h>
#if !defined(TURBO_HAVE_EXECINFO_H) && !defined(TURBO_NO_HAVE_EXECINFO_H)
	#if (defined(TURBO_PLATFORM_LINUX) || defined(TURBO_PLATFORM_OSX)) && !defined(TURBO_PLATFORM_ANDROID)
		#define TURBO_HAVE_EXECINFO_H 1
	#else
		#define TURBO_NO_HAVE_EXECINFO_H 1
	#endif
#endif

// #include <semaphore.h> (Unix semaphore support)
#if !defined(TURBO_HAVE_SEMAPHORE_H) && !defined(TURBO_NO_HAVE_SEMAPHORE_H)
	#if defined(TURBO_PLATFORM_LINUX)
		#define TURBO_HAVE_SEMAPHORE_H 1
	#else
		#define TURBO_NO_HAVE_SEMAPHORE_H 1
	#endif
#endif

// #include <dirent.h> (Unix semaphore support)
#if !defined(TURBO_HAVE_DIRENT_H) && !defined(TURBO_NO_HAVE_DIRENT_H)
	#if defined(TURBO_PLATFORM_UNIX) && !defined(TURBO_PLATFORM_CONSOLE)
		#define TURBO_HAVE_DIRENT_H 1
	#else
		#define TURBO_NO_HAVE_DIRENT_H 1
	#endif
#endif

// #include <array>, <forward_list>, <ununordered_set>, <unordered_map>
#if !defined(TURBO_HAVE_CPP11_CONTAINERS) && !defined(TURBO_NO_HAVE_CPP11_CONTAINERS)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_CONTAINERS 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4004) // Actually GCC 4.3 supports array and unordered_
		#define TURBO_HAVE_CPP11_CONTAINERS 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_CONTAINERS 1
	#else
		#define TURBO_NO_HAVE_CPP11_CONTAINERS 1
	#endif
#endif

// #include <atomic>
#if !defined(TURBO_HAVE_CPP11_ATOMIC) && !defined(TURBO_NO_HAVE_CPP11_ATOMIC)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_ATOMIC 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4007)
		#define TURBO_HAVE_CPP11_ATOMIC 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_ATOMIC 1
	#else
		#define TURBO_NO_HAVE_CPP11_ATOMIC 1
	#endif
#endif

// #include <condition_variable>
#if !defined(TURBO_HAVE_CPP11_CONDITION_VARIABLE) && !defined(TURBO_NO_HAVE_CPP11_CONDITION_VARIABLE)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_CONDITION_VARIABLE 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4007)
		#define TURBO_HAVE_CPP11_CONDITION_VARIABLE 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_CONDITION_VARIABLE 1
	#else
		#define TURBO_NO_HAVE_CPP11_CONDITION_VARIABLE 1
	#endif
#endif

// #include <mutex>
#if !defined(TURBO_HAVE_CPP11_MUTEX) && !defined(TURBO_NO_HAVE_CPP11_MUTEX)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_MUTEX 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4007)
		#define TURBO_HAVE_CPP11_MUTEX 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_MUTEX 1
	#else
		#define TURBO_NO_HAVE_CPP11_MUTEX 1
	#endif
#endif

// #include <thread>
#if !defined(TURBO_HAVE_CPP11_THREAD) && !defined(TURBO_NO_HAVE_CPP11_THREAD)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_THREAD 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4007)
		#define TURBO_HAVE_CPP11_THREAD 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_THREAD 1
	#else
		#define TURBO_NO_HAVE_CPP11_THREAD 1
	#endif
#endif

// #include <future>
#if !defined(TURBO_HAVE_CPP11_FUTURE) && !defined(TURBO_NO_HAVE_CPP11_FUTURE)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_FUTURE 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4005)
		#define TURBO_HAVE_CPP11_FUTURE 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_FUTURE 1
	#else
		#define TURBO_NO_HAVE_CPP11_FUTURE 1
	#endif
#endif


// #include <type_traits>
#if !defined(TURBO_HAVE_CPP11_TYPE_TRAITS) && !defined(TURBO_NO_HAVE_CPP11_TYPE_TRAITS)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_TYPE_TRAITS 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4007) // Prior versions of libstdc++ have incomplete support for C++11 type traits.
		#define TURBO_HAVE_CPP11_TYPE_TRAITS 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_TYPE_TRAITS 1
	#else
		#define TURBO_NO_HAVE_CPP11_TYPE_TRAITS 1
	#endif
#endif

// #include <tuple>
#if !defined(TURBO_HAVE_CPP11_TUPLES) && !defined(TURBO_NO_HAVE_CPP11_TUPLES)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_TUPLES 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4003)
		#define TURBO_HAVE_CPP11_TUPLES 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_TUPLES 1
	#else
		#define TURBO_NO_HAVE_CPP11_TUPLES 1
	#endif
#endif

// #include <regex>
#if !defined(TURBO_HAVE_CPP11_REGEX) && !defined(TURBO_NO_HAVE_CPP11_REGEX)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) && (defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_REGEX 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4003)
		#define TURBO_HAVE_CPP11_REGEX 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_REGEX 1
	#else
		#define TURBO_NO_HAVE_CPP11_REGEX 1
	#endif
#endif

// #include <random>
#if !defined(TURBO_HAVE_CPP11_RANDOM) && !defined(TURBO_NO_HAVE_CPP11_RANDOM)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_RANDOM 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4005)
		#define TURBO_HAVE_CPP11_RANDOM 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_RANDOM 1
	#else
		#define TURBO_NO_HAVE_CPP11_RANDOM 1
	#endif
#endif

// #include <chrono> 
#if !defined(TURBO_HAVE_CPP11_CHRONO) && !defined(TURBO_NO_HAVE_CPP11_CHRONO)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_CHRONO 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4007) // chrono was broken in glibc prior to 4.7.
		#define TURBO_HAVE_CPP11_CHRONO 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_CHRONO 1
	#else
		#define TURBO_NO_HAVE_CPP11_CHRONO 1
	#endif
#endif

// #include <scoped_allocator> 
#if !defined(TURBO_HAVE_CPP11_SCOPED_ALLOCATOR) && !defined(TURBO_NO_HAVE_CPP11_SCOPED_ALLOCATOR)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
		#define TURBO_HAVE_CPP11_SCOPED_ALLOCATOR 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4007)
		#define TURBO_HAVE_CPP11_SCOPED_ALLOCATOR 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_SCOPED_ALLOCATOR 1
	#else
		#define TURBO_NO_HAVE_CPP11_SCOPED_ALLOCATOR 1
	#endif
#endif

// #include <initializer_list> 
#if !defined(TURBO_HAVE_CPP11_INITIALIZER_LIST) && !defined(TURBO_NO_HAVE_CPP11_INITIALIZER_LIST)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !defined(TURBO_COMPILER_NO_INITIALIZER_LISTS) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_INITIALIZER_LIST 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_CLANG) && (TURBO_COMPILER_VERSION >= 301) && !defined(TURBO_COMPILER_NO_INITIALIZER_LISTS) && !defined(TURBO_PLATFORM_APPLE)
		#define TURBO_HAVE_CPP11_INITIALIZER_LIST 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBCPP_LIBRARY) && defined(TURBO_COMPILER_CLANG) && (TURBO_COMPILER_VERSION >= 301) && !defined(TURBO_COMPILER_NO_INITIALIZER_LISTS) && !defined(TURBO_PLATFORM_APPLE)
		#define TURBO_HAVE_CPP11_INITIALIZER_LIST 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4004) && !defined(TURBO_COMPILER_NO_INITIALIZER_LISTS) && !defined(TURBO_PLATFORM_APPLE)
		#define TURBO_HAVE_CPP11_INITIALIZER_LIST 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1) && !defined(TURBO_COMPILER_NO_INITIALIZER_LISTS)
		#define TURBO_HAVE_CPP11_INITIALIZER_LIST 1
	#else
		#define TURBO_NO_HAVE_CPP11_INITIALIZER_LIST 1
	#endif
#endif

// #include <system_error> 
#if !defined(TURBO_HAVE_CPP11_SYSTEM_ERROR) && !defined(TURBO_NO_HAVE_CPP11_SYSTEM_ERROR)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_SYSTEM_ERROR 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_CLANG) && (TURBO_COMPILER_VERSION >= 301) && !defined(TURBO_PLATFORM_APPLE)
		#define TURBO_HAVE_CPP11_SYSTEM_ERROR 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4004) && !defined(TURBO_PLATFORM_APPLE)
		#define TURBO_HAVE_CPP11_SYSTEM_ERROR 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_SYSTEM_ERROR 1
	#else
		#define TURBO_NO_HAVE_CPP11_SYSTEM_ERROR 1
	#endif
#endif

// #include <codecvt> 
#if !defined(TURBO_HAVE_CPP11_CODECVT) && !defined(TURBO_NO_HAVE_CPP11_CODECVT)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_CODECVT 1
	// Future versions of libc++ may support this header.  However, at the moment there isn't
	// a reliable way of detecting if this header is available.
	//#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4008)
	//    #define TURBO_HAVE_CPP11_CODECVT 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_CODECVT 1
	#else
		#define TURBO_NO_HAVE_CPP11_CODECVT 1
	#endif
#endif

// #include <typeindex> 
#if !defined(TURBO_HAVE_CPP11_TYPEINDEX) && !defined(TURBO_NO_HAVE_CPP11_TYPEINDEX)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_TYPEINDEX 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4006)
		#define TURBO_HAVE_CPP11_TYPEINDEX 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_TYPEINDEX 1
	#else
		#define TURBO_NO_HAVE_CPP11_TYPEINDEX 1
	#endif
#endif




/* TURBO_HAVE_XXX_DECL */

#if !defined(TURBO_HAVE_mkstemps_DECL) && !defined(TURBO_NO_HAVE_mkstemps_DECL)
	#if defined(TURBO_PLATFORM_APPLE) || defined(CS_UNDEFINED_STRING)
		#define TURBO_HAVE_mkstemps_DECL 1
	#else
		#define TURBO_NO_HAVE_mkstemps_DECL 1
	#endif
#endif

#if !defined(TURBO_HAVE_gettimeofday_DECL) && !defined(TURBO_NO_HAVE_gettimeofday_DECL)
	#if defined(TURBO_PLATFORM_POSIX) /* Posix means Linux, Unix, and Macintosh OSX, among others (including Linux-based mobile platforms). */
		#define TURBO_HAVE_gettimeofday_DECL 1
	#else
		#define TURBO_NO_HAVE_gettimeofday_DECL 1
	#endif
#endif

#if !defined(TURBO_HAVE_strcasecmp_DECL) && !defined(TURBO_NO_HAVE_strcasecmp_DECL)
	#if !defined(TURBO_PLATFORM_MICROSOFT)
		#define TURBO_HAVE_strcasecmp_DECL  1     /* This is found as stricmp when not found as strcasecmp */
		#define TURBO_HAVE_strncasecmp_DECL 1
	#else
		#define TURBO_HAVE_stricmp_DECL  1
		#define TURBO_HAVE_strnicmp_DECL 1
	#endif
#endif

#if !defined(TURBO_HAVE_mmap_DECL) && !defined(TURBO_NO_HAVE_mmap_DECL)
	#if defined(TURBO_PLATFORM_POSIX)
		#define TURBO_HAVE_mmap_DECL 1 /* mmap functionality varies significantly between systems. */
	#else
		#define TURBO_NO_HAVE_mmap_DECL 1
	#endif
#endif

#if !defined(TURBO_HAVE_fopen_DECL) && !defined(TURBO_NO_HAVE_fopen_DECL)
		#define TURBO_HAVE_fopen_DECL 1 /* C FILE functionality such as fopen */
#endif

#if !defined(TURBO_HAVE_ISNAN) && !defined(TURBO_NO_HAVE_ISNAN)
	#if defined(TURBO_PLATFORM_MICROSOFT) && !defined(TURBO_PLATFORM_MINGW)
		#define TURBO_HAVE_ISNAN(x)  _isnan(x)          /* declared in <math.h> */
		#define TURBO_HAVE_ISINF(x)  !_finite(x)
	#elif defined(TURBO_PLATFORM_APPLE)
		#define TURBO_HAVE_ISNAN(x)  std::isnan(x)      /* declared in <cmath> */
		#define TURBO_HAVE_ISINF(x)  std::isinf(x)
	#elif defined(TURBO_PLATFORM_ANDROID)
		#define TURBO_HAVE_ISNAN(x)  __builtin_isnan(x) /* There are a number of standard libraries for Android and it's hard to tell them apart, so just go with builtins */
		#define TURBO_HAVE_ISINF(x)  __builtin_isinf(x)
	#elif defined(__GNUC__) && defined(__CYGWIN__)
		#define TURBO_HAVE_ISNAN(x)  __isnand(x)        /* declared nowhere, it seems. */
		#define TURBO_HAVE_ISINF(x)  __isinfd(x)
	#else
		#define TURBO_HAVE_ISNAN(x)  std::isnan(x)      /* declared in <cmath> */
		#define TURBO_HAVE_ISINF(x)  std::isinf(x)
	#endif
#endif

#if !defined(TURBO_HAVE_itoa_DECL) && !defined(TURBO_NO_HAVE_itoa_DECL)
	#if defined(TURBO_COMPILER_MSVC)
		#define TURBO_HAVE_itoa_DECL 1
	#else
		#define TURBO_NO_HAVE_itoa_DECL 1
	#endif
#endif

#if !defined(TURBO_HAVE_nanosleep_DECL) && !defined(TURBO_NO_HAVE_nanosleep_DECL)
	#if (defined(TURBO_PLATFORM_UNIX) && !defined(TURBO_PLATFORM_SONY)) || defined(TURBO_PLATFORM_IPHONE) || defined(TURBO_PLATFORM_OSX) || defined(TURBO_PLATFORM_SONY) || defined(CS_UNDEFINED_STRING)
		#define TURBO_HAVE_nanosleep_DECL 1
	#else
		#define TURBO_NO_HAVE_nanosleep_DECL 1
	#endif
#endif

#if !defined(TURBO_HAVE_utime_DECL) && !defined(TURBO_NO_HAVE_utime_DECL)
	#if defined(TURBO_PLATFORM_MICROSOFT)
		#define TURBO_HAVE_utime_DECL _utime
	#elif TURBO_PLATFORM_UNIX
		#define TURBO_HAVE_utime_DECL utime
	#else
		#define TURBO_NO_HAVE_utime_DECL 1
	#endif
#endif

#if !defined(TURBO_HAVE_ftruncate_DECL) && !defined(TURBO_NO_HAVE_ftruncate_DECL)
	#if !defined(__MINGW32__)
		#define TURBO_HAVE_ftruncate_DECL 1
	#else
		#define TURBO_NO_HAVE_ftruncate_DECL 1
	#endif
#endif

#if !defined(TURBO_HAVE_localtime_DECL) && !defined(TURBO_NO_HAVE_localtime_DECL)
		#define TURBO_HAVE_localtime_DECL 1
#endif

#if !defined(TURBO_HAVE_pthread_getattr_np_DECL) && !defined(TURBO_NO_HAVE_pthread_getattr_np_DECL)
	#if defined(TURBO_PLATFORM_LINUX)
		#define TURBO_HAVE_pthread_getattr_np_DECL 1
	#else
		#define TURBO_NO_HAVE_pthread_getattr_np_DECL 1
	#endif
#endif



/* TURBO_HAVE_XXX_IMPL*/

#if !defined(TURBO_HAVE_WCHAR_IMPL) && !defined(TURBO_NO_HAVE_WCHAR_IMPL)
	#if defined(TURBO_PLATFORM_DESKTOP)
		#define TURBO_HAVE_WCHAR_IMPL 1      /* Specifies if wchar_t string functions are provided, such as wcslen, wprintf, etc. Implies TURBO_HAVE_WCHAR_H */
	#else
		#define TURBO_NO_HAVE_WCHAR_IMPL 1
	#endif
#endif

#if !defined(TURBO_HAVE_getenv_IMPL) && !defined(TURBO_NO_HAVE_getenv_IMPL)
	#if (defined(TURBO_PLATFORM_DESKTOP) || defined(TURBO_PLATFORM_UNIX)) && !defined(TURBO_PLATFORM_WINRT)
		#define TURBO_HAVE_getenv_IMPL 1
	#else
		#define TURBO_NO_HAVE_getenv_IMPL 1
	#endif
#endif

#if !defined(TURBO_HAVE_setenv_IMPL) && !defined(TURBO_NO_HAVE_setenv_IMPL)
	#if defined(TURBO_PLATFORM_UNIX) && defined(TURBO_PLATFORM_POSIX)
		#define TURBO_HAVE_setenv_IMPL 1
	#else
		#define TURBO_NO_HAVE_setenv_IMPL 1
	#endif
#endif

#if !defined(TURBO_HAVE_unsetenv_IMPL) && !defined(TURBO_NO_HAVE_unsetenv_IMPL)
	#if defined(TURBO_PLATFORM_UNIX) && defined(TURBO_PLATFORM_POSIX)
		#define TURBO_HAVE_unsetenv_IMPL 1
	#else
		#define TURBO_NO_HAVE_unsetenv_IMPL 1
	#endif
#endif

#if !defined(TURBO_HAVE_putenv_IMPL) && !defined(TURBO_NO_HAVE_putenv_IMPL)
	#if (defined(TURBO_PLATFORM_DESKTOP) || defined(TURBO_PLATFORM_UNIX)) && !defined(TURBO_PLATFORM_WINRT)
		#define TURBO_HAVE_putenv_IMPL 1        /* With Microsoft compilers you may need to use _putenv, as they have deprecated putenv. */
	#else
		#define TURBO_NO_HAVE_putenv_IMPL 1
	#endif
#endif

#if !defined(TURBO_HAVE_time_IMPL) && !defined(TURBO_NO_HAVE_time_IMPL)
		#define TURBO_HAVE_time_IMPL 1
		#define TURBO_HAVE_clock_IMPL 1
#endif

// <cstdio> fopen()
#if !defined(TURBO_HAVE_fopen_IMPL) && !defined(TURBO_NO_HAVE_fopen_IMPL)
		#define TURBO_HAVE_fopen_IMPL 1  /* C FILE functionality such as fopen */
#endif

// <arpa/inet.h> inet_ntop()
#if !defined(TURBO_HAVE_inet_ntop_IMPL) && !defined(TURBO_NO_HAVE_inet_ntop_IMPL)
	#if (defined(TURBO_PLATFORM_UNIX) || defined(TURBO_PLATFORM_POSIX)) && !defined(TURBO_PLATFORM_SONY) && !defined(CS_UNDEFINED_STRING)
		#define TURBO_HAVE_inet_ntop_IMPL 1  /* This doesn't identify if the platform SDK has some alternative function that does the same thing; */
		#define TURBO_HAVE_inet_pton_IMPL 1  /* it identifies strictly the <arpa/inet.h> inet_ntop and inet_pton functions. For example, Microsoft has InetNtop in <Ws2tcpip.h> */
	#else
		#define TURBO_NO_HAVE_inet_ntop_IMPL 1
		#define TURBO_NO_HAVE_inet_pton_IMPL 1
	#endif
#endif

// <time.h> clock_gettime()
#if !defined(TURBO_HAVE_clock_gettime_IMPL) && !defined(TURBO_NO_HAVE_clock_gettime_IMPL)
	#if defined(TURBO_PLATFORM_LINUX) || defined(__CYGWIN__) || (defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)) || (defined(TURBO_PLATFORM_POSIX) && defined(_CPPLIB_VER) /*Dinkumware*/)
		#define TURBO_HAVE_clock_gettime_IMPL 1 /* You need to link the 'rt' library to get this */
	#else
		#define TURBO_NO_HAVE_clock_gettime_IMPL 1
	#endif
#endif

#if !defined(TURBO_HAVE_getcwd_IMPL) && !defined(TURBO_NO_HAVE_getcwd_IMPL)
	#if (defined(TURBO_PLATFORM_DESKTOP) || defined(TURBO_PLATFORM_UNIX)) && !defined(TURBO_PLATFORM_ANDROID) && !defined(TURBO_PLATFORM_WINRT)
		#define TURBO_HAVE_getcwd_IMPL 1       /* With Microsoft compilers you may need to use _getcwd, as they have deprecated getcwd. And in any case it's present at <direct.h> */
	#else
		#define TURBO_NO_HAVE_getcwd_IMPL 1
	#endif
#endif

#if !defined(TURBO_HAVE_tmpnam_IMPL) && !defined(TURBO_NO_HAVE_tmpnam_IMPL)
	#if (defined(TURBO_PLATFORM_DESKTOP) || defined(TURBO_PLATFORM_UNIX)) && !defined(TURBO_PLATFORM_ANDROID)
		#define TURBO_HAVE_tmpnam_IMPL 1
	#else
		#define TURBO_NO_HAVE_tmpnam_IMPL 1
	#endif
#endif

// nullptr, the built-in C++11 type.
// This TURBO_HAVE is deprecated, as TURBO_COMPILER_NO_NULLPTR is more appropriate, given that nullptr is a compiler-level feature and not a library feature.
#if !defined(TURBO_HAVE_nullptr_IMPL) && !defined(TURBO_NO_HAVE_nullptr_IMPL)
	#if defined(TURBO_COMPILER_NO_NULLPTR)
		#define TURBO_NO_HAVE_nullptr_IMPL 1
	#else
		#define TURBO_HAVE_nullptr_IMPL 1
	#endif
#endif

// <cstddef> std::nullptr_t
// Note that <TBBase/nullptr.h> implements a portable nullptr implementation, but this
// TURBO_HAVE specifically refers to std::nullptr_t from the standard libraries.
#if !defined(TURBO_HAVE_nullptr_t_IMPL) && !defined(TURBO_NO_HAVE_nullptr_t_IMPL)
	#if defined(TURBO_COMPILER_CPP11_ENABLED)
		// VS2010+ with its default Dinkumware standard library.
		#if defined(_MSC_VER) && (_MSC_VER >= 1600) && defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY)
			#define TURBO_HAVE_nullptr_t_IMPL 1

		#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) // clang/llvm libc++
			#define TURBO_HAVE_nullptr_t_IMPL 1

		#elif defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) // GNU libstdc++
			// Unfortunately __GLIBCXX__ date values don't go strictly in version ordering.
			#if (__GLIBCXX__ >= 20110325) && (__GLIBCXX__ != 20120702) && (__GLIBCXX__ != 20110428)
				#define TURBO_HAVE_nullptr_t_IMPL 1
			#else
				#define TURBO_NO_HAVE_nullptr_t_IMPL 1
			#endif
			
		// We simply assume that the standard library (e.g. Dinkumware) provides std::nullptr_t.
		#elif defined(__clang__)
			#define TURBO_HAVE_nullptr_t_IMPL 1

		// With GCC compiler >= 4.6, std::nullptr_t is always defined in <cstddef>, in practice.
		#elif defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4006)
			#define TURBO_HAVE_nullptr_t_IMPL 1

		// The EDG compiler provides nullptr, but uses an older standard library that doesn't support std::nullptr_t.
		#elif defined(__EDG_VERSION__) && (__EDG_VERSION__ >= 403) 
			#define TURBO_HAVE_nullptr_t_IMPL 1
			
		#else
			#define TURBO_NO_HAVE_nullptr_t_IMPL 1
		#endif
	#else
		#define TURBO_NO_HAVE_nullptr_t_IMPL 1
	#endif
#endif

// <exception> std::terminate
#if !defined(TURBO_HAVE_std_terminate_IMPL) && !defined(TURBO_NO_HAVE_std_terminate_IMPL)
	#if !defined(TURBO_PLATFORM_IPHONE) && !defined(TURBO_PLATFORM_ANDROID)
		#define TURBO_HAVE_std_terminate_IMPL 1 /* iOS doesn't appear to provide an implementation for std::terminate under the armv6 target. */
	#else
		#define TURBO_NO_HAVE_std_terminate_IMPL 1
	#endif
#endif

// <iterator>: std::begin, std::end, std::prev, std::next, std::move_iterator.
#if !defined(TURBO_HAVE_CPP11_ITERATOR_IMPL) && !defined(TURBO_NO_HAVE_CPP11_ITERATOR_IMPL)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_ITERATOR_IMPL 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4006)
		#define TURBO_HAVE_CPP11_ITERATOR_IMPL 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_ITERATOR_IMPL 1
	#else
		#define TURBO_NO_HAVE_CPP11_ITERATOR_IMPL 1
	#endif
#endif

// <memory>: std::weak_ptr, std::shared_ptr, std::unique_ptr, std::bad_weak_ptr, std::owner_less
#if !defined(TURBO_HAVE_CPP11_SMART_POINTER_IMPL) && !defined(TURBO_NO_HAVE_CPP11_SMART_POINTER_IMPL)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_SMART_POINTER_IMPL 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4004)
		#define TURBO_HAVE_CPP11_SMART_POINTER_IMPL 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_SMART_POINTER_IMPL 1
	#else
		#define TURBO_NO_HAVE_CPP11_SMART_POINTER_IMPL 1
	#endif
#endif

// <functional>: std::function, std::mem_fn, std::bad_function_call, std::is_bind_expression, std::is_placeholder, std::reference_wrapper, std::hash, std::bind, std::ref, std::cref.
#if !defined(TURBO_HAVE_CPP11_FUNCTIONAL_IMPL) && !defined(TURBO_NO_HAVE_CPP11_FUNCTIONAL_IMPL)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_FUNCTIONAL_IMPL 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4004)
		#define TURBO_HAVE_CPP11_FUNCTIONAL_IMPL 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_FUNCTIONAL_IMPL 1
	#else
		#define TURBO_NO_HAVE_CPP11_FUNCTIONAL_IMPL 1
	#endif
#endif

// <exception> std::current_exception, std::rethrow_exception, std::exception_ptr, std::make_exception_ptr
#if !defined(TURBO_HAVE_CPP11_EXCEPTION_IMPL) && !defined(TURBO_NO_HAVE_CPP11_EXCEPTION_IMPL)
	#if defined(TURBO_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
		#define TURBO_HAVE_CPP11_EXCEPTION_IMPL 1
	#elif defined(TURBO_COMPILER_CPP11_ENABLED) && defined(TURBO_HAVE_LIBSTDCPP_LIBRARY) && defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4004)
		#define TURBO_HAVE_CPP11_EXCEPTION_IMPL 1
	#elif defined(TURBO_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
		#define TURBO_HAVE_CPP11_EXCEPTION_IMPL 1
	#else
		#define TURBO_NO_HAVE_CPP11_EXCEPTION_IMPL 1
	#endif
#endif




/* Implementations that all platforms seem to have: */
/*
	alloca
	malloc
	calloc
	strtoll
	strtoull
	vsprintf
	vsnprintf
*/

/* Implementations that we don't care about: */
/*
	bcopy   -- Just use memmove or some customized equivalent. bcopy offers no practical benefit.
	strlcpy -- So few platforms have this built-in that we get no benefit from using it. Use turbo::platform::Strlcpy instead.
	strlcat -- "
*/



/*-----------------------------------------------------------------------------
	TBBASE_USER_HAVE_HEADER
	
	This allows the user to define a header file to be #included after the 
	eahave.h's contents are compiled. A primary use of this is to override
	the contents of this header file. You can define the overhead header 
	file name in-code or define it globally as part of your build file.
	
	Example usage:
	   #define TBBASE_USER_HAVE_HEADER "MyHaveOverrides.h"
	   #include <TBBase/eahave.h>
---------------------------------------------------------------------------*/

#ifdef TBBASE_USER_HAVE_HEADER
	#include TBBASE_USER_HAVE_HEADER
#endif


#endif  // TURBO_PLATFORM_CONFIG_HAVE_H_



