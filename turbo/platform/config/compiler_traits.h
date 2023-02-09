/*-----------------------------------------------------------------------------
 * config/compiler_traits.h
 *
 * Copyright (c) Electronic Arts Inc. All rights reserved.
 * Copyright (c) Jeff.Li
 *-----------------------------------------------------------------------------
 * Currently supported defines include:
 *    TURBO_PREPROCESSOR_JOIN
 *    
 *    TURBO_COMPILER_IS_ANSIC
 *    TURBO_COMPILER_IS_C99
 *    TURBO_COMPILER_IS_C11
 *    TURBO_COMPILER_HAS_C99_TYPES
 *    TURBO_COMPILER_IS_CPLUSPLUS
 *    TURBO_COMPILER_MANAGED_CPP
 *    TURBO_COMPILER_INTMAX_SIZE
 *    TURBO_OFFSETOF
 *    TURBO_SIZEOF_MEMBER
 *
 *    TURBO_ALIGN_OF()
 *    TURBO_ALIGN_MAX_STATIC / TURBO_ALIGN_MAX_AUTOMATIC
 *    TURBO_ALIGN() / TURBO_PREFIX_ALIGN() / TURBO_POSTFIX_ALIGN()
 *    TURBO_ALIGNED()
 *    TURBO_PACKED()
 *
 *    TURBO_LIKELY()
 *    TURBO_UNLIKELY()
 *    TURBO_INIT_PRIORITY()
 *    TURBO_MAY_ALIAS()
 *    TURBO_ASSUME()
 *    TURBO_ANALYSIS_ASSUME()
 *    TURBO_PURE
 *    TURBO_WEAK
 *    TURBO_UNUSED()
 *    TURBO_EMPTY()
 *
 *    TURBO_WCHAR_T_NON_NATIVE
 *    TURBO_WCHAR_SIZE = <n bytes>
 *
 *    TURBO_RESTRICT
 *    TURBO_DEPRECATED   / TURBO_PREFIX_DEPRECATED   / TURBO_POSTFIX_DEPRECATED
 *    TURBO_FORCE_INLINE / TURBO_PREFIX_FORCE_INLINE / TURBO_POSTFIX_FORCE_INLINE
 *    TURBO_NO_INLINE    / TURBO_PREFIX_NO_INLINE    / TURBO_POSTFIX_NO_INLINE
 *    TURBO_NO_VTABLE    / TURBO_CLASS_NO_VTABLE     / TURBO_STRUCT_NO_VTABLE
 *    TURBO_PASCAL
 *    TURBO_PASCAL_FUNC()
 *    TURBO_SSE = [0 | 1]
 *    TURBO_IMPORT
 *    TURBO_EXPORT
 *    TURBO_PRAGMA_ONCE_SUPPORTED
 *    TURBO_ONCE
 *    TURBO_OVERRIDE
 *    TURBO_INHERITANCE_FINAL
 *    TURBO_SEALED
 *    TURBO_ABSTRACT
 *    TURBO_CONSTEXPR / TURBO_CONSTEXPR_OR_CONST
 *    TURBO_CONSTEXPR_IF
 *    TURBO_EXTERN_TEMPLATE
 *    TURBO_NOEXCEPT
 *    TURBO_NORETURN
 *    TURBO_CARRIES_DEPENDENCY
 *    TURBO_NON_COPYABLE / struct EANonCopyable
 *    TURBO_OPTIMIZE_OFF / TURBO_OPTIMIZE_ON
 *    TURBO_SIGNED_RIGHT_SHIFT_IS_UNSIGNED
 *
 *    TURBO_DISABLE_VC_WARNING    / TURBO_RESTORE_VC_WARNING / TURBO_DISABLE_ALL_VC_WARNINGS / TURBO_RESTORE_ALL_VC_WARNINGS
 *    TURBO_DISABLE_GCC_WARNING   / TURBO_RESTORE_GCC_WARNING
 *    TURBO_DISABLE_CLANG_WARNING / TURBO_RESTORE_CLANG_WARNING
 *    TURBO_DISABLE_SN_WARNING    / TURBO_RESTORE_SN_WARNING / TURBO_DISABLE_ALL_SN_WARNINGS / TURBO_RESTORE_ALL_SN_WARNINGS
 *    TURBO_DISABLE_GHS_WARNING   / TURBO_RESTORE_GHS_WARNING
 *    TURBO_DISABLE_EDG_WARNING   / TURBO_RESTORE_EDG_WARNING
 *    TURBO_DISABLE_CW_WARNING    / TURBO_RESTORE_CW_WARNING
 *
 *    TURBO_DISABLE_DEFAULT_CTOR
 *    TURBO_DISABLE_COPY_CTOR
 *    TURBO_DISABLE_MOVE_CTOR
 *    TURBO_DISABLE_ASSIGNMENT_OPERATOR
 *    TURBO_DISABLE_MOVE_OPERATOR
 *
 *  Todo:
 *    Find a way to reliably detect wchar_t size at preprocessor time and 
 *    implement it below for TURBO_WCHAR_SIZE.
 *
 *  Todo:
 *    Find out how to support TURBO_PASCAL and TURBO_PASCAL_FUNC for systems in
 *    which it hasn't yet been found out for.
 *---------------------------------------------------------------------------*/


#ifndef TURBO_PLATFORM_CONFIG_COMPILER_TRAITS_H_
#define TURBO_PLATFORM_CONFIG_COMPILER_TRAITS_H_

	#include "turbo/platform/config/platform.h"
	#include "turbo/platform/config/compiler.h"


    // TURBO_HAVE_ATTRIBUTE
    //
    // A function-like feature checking macro that is a wrapper around
    // `__has_attribute`, which is defined by GCC 5+ and Clang and evaluates to a
    // nonzero constant integer if the attribute is supported or 0 if not.
    //
    // It evaluates to zero if `__has_attribute` is not defined by the compiler.
    //
    // GCC: https://gcc.gnu.org/gcc-5/changes.html
    // Clang: https://clang.llvm.org/docs/LanguageExtensions.html
    #ifdef __has_attribute
    #define TURBO_HAVE_ATTRIBUTE(x) __has_attribute(x)
    #else
    #define TURBO_HAVE_ATTRIBUTE(x) 0
    #endif

    // TURBO_HAVE_CPP_ATTRIBUTE
    //
    // A function-like feature checking macro that accepts C++11 style attributes.
    // It's a wrapper around `__has_cpp_attribute`, defined by ISO C++ SD-6
    // (https://en.cppreference.com/w/cpp/experimental/feature_test). If we don't
    // find `__has_cpp_attribute`, will evaluate to 0.
    #if defined(__cplusplus) && defined(__has_cpp_attribute)
    // NOTE: requiring __cplusplus above should not be necessary, but
    // works around https://bugs.llvm.org/show_bug.cgi?id=23435.
    #define TURBO_HAVE_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
    #else
    #define TURBO_HAVE_CPP_ATTRIBUTE(x) 0
    #endif



    // -----------------------------------------------------------------------------
    // Function Attributes
    // -----------------------------------------------------------------------------
    //
    // GCC: https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
    // Clang: https://clang.llvm.org/docs/AttributeReference.html

    // TURBO_PRINTF_ATTRIBUTE
    // TURBO_SCANF_ATTRIBUTE
    //
    // Tells the compiler to perform `printf` format string checking if the
    // compiler supports it; see the 'format' attribute in
    // <https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Function-Attributes.html>.
    //
    // Note: As the GCC manual states, "[s]ince non-static C++ methods
    // have an implicit 'this' argument, the arguments of such methods
    // should be counted from two, not one."
    #if TURBO_HAVE_ATTRIBUTE(format) || (defined(__GNUC__) && !defined(__clang__))
    #define TURBO_PRINTF_ATTRIBUTE(string_index, first_to_check) \
      __attribute__((__format__(__printf__, string_index, first_to_check)))
    #define TURBO_SCANF_ATTRIBUTE(string_index, first_to_check) \
      __attribute__((__format__(__scanf__, string_index, first_to_check)))
    #else
    #define TURBO_PRINTF_ATTRIBUTE(string_index, first_to_check)
    #define TURBO_SCANF_ATTRIBUTE(string_index, first_to_check)
    #endif

	// Metrowerks uses #defines in its core C header files to define
	// the kind of information we need below (e.g. C99 compatibility)



	// Determine if this compiler is ANSI C compliant and if it is C99 compliant.
	#if defined(__STDC__)
		#define TURBO_COMPILER_IS_ANSIC 1    // The compiler claims to be ANSI C

		// Is the compiler a C99 compiler or equivalent?
		// From ISO/IEC 9899:1999:
		//    6.10.8 Predefined macro names
		//    __STDC_VERSION__ The integer constant 199901L. (150)
		//
		//    150) This macro was not specified in ISO/IEC 9899:1990 and was
		//    specified as 199409L in ISO/IEC 9899/AMD1:1995. The intention
		//    is that this will remain an integer constant of type long int
		//    that is increased with each revision of this International Standard.
		//
		#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
			#define TURBO_COMPILER_IS_C99 1
		#endif

 		// Is the compiler a C11 compiler?
 		// From ISO/IEC 9899:2011:
		//   Page 176, 6.10.8.1 (Predefined macro names) :
 		//   __STDC_VERSION__ The integer constant 201112L. (178)
		//
		#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
			#define TURBO_COMPILER_IS_C11 1
		#endif
	#endif

	// Some compilers (e.g. GCC) define __USE_ISOC99 if they are not
	// strictly C99 compilers (or are simply C++ compilers) but are set
	// to use C99 functionality. Metrowerks defines _MSL_C99 as 1 in
	// this case, but 0 otherwise.
	#if (defined(__USE_ISOC99) || (defined(_MSL_C99) && (_MSL_C99 == 1))) && !defined(TURBO_COMPILER_IS_C99)
		#define TURBO_COMPILER_IS_C99 1
	#endif

	// Metrowerks defines C99 types (e.g. intptr_t) instrinsically when in C99 mode (-lang C99 on the command line).
	#if (defined(_MSL_C99) && (_MSL_C99 == 1))
		#define TURBO_COMPILER_HAS_C99_TYPES 1
	#endif

	#if defined(__GNUC__)
		#if (((__GNUC__ * 100) + __GNUC_MINOR__) >= 302) // Also, GCC defines _HAS_C9X.
			#define TURBO_COMPILER_HAS_C99_TYPES 1 // The compiler is not necessarily a C99 compiler, but it defines C99 types.

			#ifndef __STDC_LIMIT_MACROS
				#define __STDC_LIMIT_MACROS 1
			#endif

			#ifndef __STDC_CONSTANT_MACROS
				#define __STDC_CONSTANT_MACROS 1    // This tells the GCC compiler that we want it to use its native C99 types.
			#endif
		#endif
	#endif

	#if defined(_MSC_VER) && (_MSC_VER >= 1600)
		#define TURBO_COMPILER_HAS_C99_TYPES 1
	#endif

	#ifdef  __cplusplus
		#define TURBO_COMPILER_IS_CPLUSPLUS 1
	#endif


	// ------------------------------------------------------------------------
	// TURBO_PREPROCESSOR_JOIN
	//
	// This macro joins the two arguments together, even when one of
	// the arguments is itself a macro (see 16.3.1 in C++98 standard).
	// This is often used to create a unique name with __LINE__.
	//
	// For example, this declaration:
	//    char TURBO_PREPROCESSOR_JOIN(unique_, __LINE__);
	// expands to this:
	//    char unique_73;
	//
	// Note that all versions of MSVC++ up to at least version 7.1
	// fail to properly compile macros that use __LINE__ in them
	// when the "program database for edit and continue" option
	// is enabled. The result is that __LINE__ gets converted to
	// something like __LINE__(Var+37).
	//
	#ifndef TURBO_PREPROCESSOR_JOIN
		#define TURBO_PREPROCESSOR_JOIN(a, b)  TURBO_PREPROCESSOR_JOIN1(a, b)
		#define TURBO_PREPROCESSOR_JOIN1(a, b) TURBO_PREPROCESSOR_JOIN2(a, b)
		#define TURBO_PREPROCESSOR_JOIN2(a, b) a##b
	#endif

    // TURBO_PRETTY_FUNCTION
    //
    // In C++11, __func__ gives the undecorated name of the current function.  That
    // is, "main", not "int main()".  Various compilers give extra macros to get the
    // decorated function name, including return type and arguments, to
    // differentiate between overload sets.  TURBO_PRETTY_FUNCTION is a portable
    // version of these macros which forwards to the correct macro on each compiler.
    #if defined(_MSC_VER)
        #define TURBO_PRETTY_FUNCTION __FUNCSIG__
    #elif defined(__GNUC__)
        #define TURBO_PRETTY_FUNCTION __PRETTY_FUNCTION__
    #else
        #error "Unsupported compiler"
    #endif

	// ------------------------------------------------------------------------
	// TURBO_STRINGIFY
	//
	// Example usage:
	//     printf("Line: %s", TURBO_STRINGIFY(__LINE__));
	//
	#ifndef TURBO_STRINGIFY
		#define TURBO_STRINGIFY(x)     TURBO_STRINGIFYIMPL(x)
		#define TURBO_STRINGIFYIMPL(x) #x
	#endif


	// ------------------------------------------------------------------------
	// TURBO_IDENTITY
	//
	#ifndef TURBO_IDENTITY
		#define TURBO_IDENTITY(x) x
	#endif


	// ------------------------------------------------------------------------
	// TURBO_COMPILER_MANAGED_CPP
	// Defined if this is being compiled with Managed C++ extensions
	#ifdef TURBO_COMPILER_MSVC
		#if TURBO_COMPILER_VERSION >= 1300
			#ifdef _MANAGED
				#define TURBO_COMPILER_MANAGED_CPP 1
			#endif
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_COMPILER_INTMAX_SIZE
	//
	// This is related to the concept of intmax_t uintmax_t, but is available
	// in preprocessor form as opposed to compile-time form. At compile-time
	// you can use intmax_t and uintmax_t to use the actual types.
	//
	#if defined(__GNUC__) && defined(__x86_64__)
		#define TURBO_COMPILER_INTMAX_SIZE 16  // intmax_t is __int128_t (GCC extension) and is 16 bytes.
	#else
		#define TURBO_COMPILER_INTMAX_SIZE 8   // intmax_t is int64_t and is 8 bytes.
	#endif



	// ------------------------------------------------------------------------
	// TURBO_LPAREN / TURBO_RPAREN / TURBO_COMMA / TURBO_SEMI
	//
	// These are used for using special characters in macro-using expressions.
	// Note that this macro intentionally uses (), as in some cases it can't
	// work unless it does.
	//
	// Example usage:
	//     int x = SOME_MACRO(SomeTemplate<int TURBO_COMMA() int EACOMMA() char>);
	//
	#ifndef TURBO_LPAREN
		#define TURBO_LPAREN() (
	#endif
	#ifndef TURBO_RPAREN
		#define TURBO_RPAREN() )
	#endif
	#ifndef TURBO_COMMA
		#define TURBO_COMMA()  ,
	#endif
	#ifndef TURBO_SEMI
		#define TURBO_SEMI()   ;
	#endif




	// ------------------------------------------------------------------------
	// TURBO_OFFSETOF
	// Implements a portable version of the non-standard offsetof macro.
	//
	// The offsetof macro is guaranteed to only work with POD types. However, we wish to use
	// it for non-POD types but where we know that offsetof will still work for the cases
	// in which we use it. GCC unilaterally gives a warning when using offsetof with a non-POD,
	// even if the given usage happens to work. So we make a workaround version of offsetof
	// here for GCC which has the same effect but tricks the compiler into not issuing the warning.
	// The 65536 does the compiler fooling; the reinterpret_cast prevents the possibility of
	// an overloaded operator& for the class getting in the way.
	//
	// Example usage:
	//     struct A{ int x; int y; };
	//     size_t n = TURBO_OFFSETOF(A, y);
	//
	#if defined(__GNUC__)                       // We can't use GCC 4's __builtin_offsetof because it mistakenly complains about non-PODs that are really PODs.
		#define TURBO_OFFSETOF(struct_, member_)  ((size_t)(((uintptr_t)&reinterpret_cast<const volatile char&>((((struct_*)65536)->member_))) - 65536))
	#else
		#define TURBO_OFFSETOF(struct_, member_)  offsetof(struct_, member_)
	#endif

	// ------------------------------------------------------------------------
	// TURBO_SIZEOF_MEMBER
	// Implements a portable way to determine the size of a member.
	//
	// The TURBO_SIZEOF_MEMBER simply returns the size of a member within a class or struct; member
	// access rules still apply. We offer two approaches depending on the compiler's support for non-static member
	// initializers although most C++11 compilers support this.
	//
	// Example usage:
	//     struct A{ int x; int y; };
	//     size_t n = TURBO_SIZEOF_MEMBER(A, y);
	//
	#ifndef TURBO_COMPILER_NO_EXTENDED_SIZEOF
		#define TURBO_SIZEOF_MEMBER(struct_, member_) (sizeof(struct_::member_))
	#else
		#define TURBO_SIZEOF_MEMBER(struct_, member_) (sizeof(((struct_*)0)->member_))
	#endif

	// ------------------------------------------------------------------------
	// alignment expressions
	//
	// Here we define
	//    TURBO_ALIGN_OF(type)         // Returns size_t.
	//    TURBO_ALIGN_MAX_STATIC       // The max align value that the compiler will respect for TURBO_ALIGN for static data (global and static variables). Some compilers allow high values, some allow no more than 8. TURBO_ALIGN_MIN is assumed to be 1.
	//    TURBO_ALIGN_MAX_AUTOMATIC    // The max align value for automatic variables (variables declared as local to a function).
	//    TURBO_ALIGN(n)               // Used as a prefix. n is byte alignment, with being a power of two. Most of the time you can use this and avoid using TURBO_PREFIX_ALIGN/TURBO_POSTFIX_ALIGN.
	//    TURBO_ALIGNED(t, v, n)       // Type, variable, alignment. Used to align an instance. You should need this only for unusual compilers.
	//    TURBO_PACKED                 // Specifies that the given structure be packed (and not have its members aligned).
	//
	// Also we define the following for rare cases that it's needed.
	//    TURBO_PREFIX_ALIGN(n)        // n is byte alignment, with being a power of two. You should need this only for unusual compilers.
	//    TURBO_POSTFIX_ALIGN(n)       // Valid values for n are 1, 2, 4, 8, etc. You should need this only for unusual compilers.
	//
	// Example usage:
	//    size_t x = TURBO_ALIGN_OF(int);                                  Non-aligned equivalents.        Meaning
	//    TURBO_PREFIX_ALIGN(8) int x = 5;                                 int x = 5;                      Align x on 8 for compilers that require prefix attributes. Can just use TURBO_ALIGN instead.
	//    TURBO_ALIGN(8) int x;                                            int x;                          Align x on 8 for compilers that allow prefix attributes.
	//    int x TURBO_POSTFIX_ALIGN(8);                                    int x;                          Align x on 8 for compilers that require postfix attributes.
	//    int x TURBO_POSTFIX_ALIGN(8) = 5;                                int x = 5;                      Align x on 8 for compilers that require postfix attributes.
	//    int x TURBO_POSTFIX_ALIGN(8)(5);                                 int x(5);                       Align x on 8 for compilers that require postfix attributes.
	//    struct TURBO_PREFIX_ALIGN(8) X { int x; } TURBO_POSTFIX_ALIGN(8);   struct X { int x; };            Define X as a struct which is aligned on 8 when used.
	//    TURBO_ALIGNED(int, x, 8) = 5;                                    int x = 5;                      Align x on 8.
	//    TURBO_ALIGNED(int, x, 16)(5);                                    int x(5);                       Align x on 16.
	//    TURBO_ALIGNED(int, x[3], 16);                                    int x[3];                       Align x array on 16.
	//    TURBO_ALIGNED(int, x[3], 16) = { 1, 2, 3 };                      int x[3] = { 1, 2, 3 };         Align x array on 16.
	//    int x[3] TURBO_PACKED;                                           int x[3];                       Pack the 3 ints of the x array. GCC doesn't seem to support packing of int arrays.
	//    struct TURBO_ALIGN(32) X { int x; int y; };                      struct X { int x; };            Define A as a struct which is aligned on 32 when used.
	//    TURBO_ALIGN(32) struct X { int x; int y; } Z;                    struct X { int x; } Z;          Define A as a struct, and align the instance Z on 32.
	//    struct X { int x TURBO_PACKED; int y TURBO_PACKED; };               struct X { int x; int y; };     Pack the x and y members of struct X.
	//    struct X { int x; int y; } TURBO_PACKED;                         struct X { int x; int y; };     Pack the members of struct X.
	//    typedef TURBO_ALIGNED(int, int16, 16); int16 n16;                typedef int int16; int16 n16;   Define int16 as an int which is aligned on 16.
	//    typedef TURBO_ALIGNED(X, X16, 16); X16 x16;                      typedef X X16; X16 x16;         Define X16 as an X which is aligned on 16.

	#if !defined(TURBO_ALIGN_MAX)                              // If the user hasn't globally set an alternative value...
		#if defined(TURBO_PROCESSOR_ARM)                       // ARM compilers in general tend to limit automatic variables to 8 or less.
			#define TURBO_ALIGN_MAX_STATIC    1048576
			#define TURBO_ALIGN_MAX_AUTOMATIC       1          // Typically they support only built-in natural aligment types (both arm-eabi and apple-abi).
		#elif defined(TURBO_PLATFORM_APPLE)
			#define TURBO_ALIGN_MAX_STATIC    1048576
			#define TURBO_ALIGN_MAX_AUTOMATIC      16
		#else
			#define TURBO_ALIGN_MAX_STATIC    1048576          // Arbitrarily high value. What is the actual max?
			#define TURBO_ALIGN_MAX_AUTOMATIC 1048576
		#endif
	#endif

	// EDG intends to be compatible with GCC but has a bug whereby it
	// fails to support calling a constructor in an aligned declaration when
	// using postfix alignment attributes. Prefix works for alignment, but does not align
	// the size like postfix does.  Prefix also fails on templates.  So gcc style post fix
	// is still used, but the user will need to use TURBO_POSTFIX_ALIGN before the constructor parameters.
	#if defined(__GNUC__) && (__GNUC__ < 3)
		#define TURBO_ALIGN_OF(type) ((size_t)__alignof__(type))
		#define TURBO_ALIGN(n)
		#define TURBO_PREFIX_ALIGN(n)
		#define TURBO_POSTFIX_ALIGN(n) __attribute__((aligned(n)))
		#define TURBO_ALIGNED(variable_type, variable, n) variable_type variable __attribute__((aligned(n)))
		#define TURBO_PACKED __attribute__((packed))

	// GCC 3.x+, IBM, and clang support prefix attributes.
	#elif (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__xlC__) || defined(__clang__)
		#define TURBO_ALIGN_OF(type) ((size_t)__alignof__(type))
		#define TURBO_ALIGN(n) __attribute__((aligned(n)))
		#define TURBO_PREFIX_ALIGN(n)
		#define TURBO_POSTFIX_ALIGN(n) __attribute__((aligned(n)))
		#define TURBO_ALIGNED(variable_type, variable, n) variable_type variable __attribute__((aligned(n)))
		#define TURBO_PACKED __attribute__((packed))

	// Metrowerks supports prefix attributes.
	// Metrowerks does not support packed alignment attributes.
	#elif defined(TURBO_COMPILER_INTEL) || defined(CS_UNDEFINED_STRING) || (defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION >= 1300))
		#define TURBO_ALIGN_OF(type) ((size_t)__alignof(type))
		#define TURBO_ALIGN(n) __declspec(align(n))
		#define TURBO_PREFIX_ALIGN(n) TURBO_ALIGN(n)
		#define TURBO_POSTFIX_ALIGN(n)
		#define TURBO_ALIGNED(variable_type, variable, n) TURBO_ALIGN(n) variable_type variable
		#define TURBO_PACKED // See TURBO_PRAGMA_PACK_VC for an alternative.

	// Arm brand compiler
	#elif defined(TURBO_COMPILER_ARM)
		#define TURBO_ALIGN_OF(type) ((size_t)__ALIGNOF__(type))
		#define TURBO_ALIGN(n) __align(n)
		#define TURBO_PREFIX_ALIGN(n) __align(n)
		#define TURBO_POSTFIX_ALIGN(n)
		#define TURBO_ALIGNED(variable_type, variable, n) __align(n) variable_type variable
		#define TURBO_PACKED __packed

	#else // Unusual compilers
		// There is nothing we can do about some of these. This is not as bad a problem as it seems.
		// If the given platform/compiler doesn't support alignment specifications, then it's somewhat
		// likely that alignment doesn't matter for that platform. Otherwise they would have defined
		// functionality to manipulate alignment.
		#define TURBO_ALIGN(n)
		#define TURBO_PREFIX_ALIGN(n)
		#define TURBO_POSTFIX_ALIGN(n)
		#define TURBO_ALIGNED(variable_type, variable, n) variable_type variable
		#define TURBO_PACKED

		#ifdef __cplusplus
			template <typename T> struct EAAlignOf1 { enum { s = sizeof (T), value = s ^ (s & (s - 1)) }; };
			template <typename T> struct EAAlignOf2;
			template <int size_diff> struct helper { template <typename T> struct Val { enum { value = size_diff }; }; };
			template <> struct helper<0> { template <typename T> struct Val { enum { value = EAAlignOf2<T>::value }; }; };
			template <typename T> struct EAAlignOf2 { struct Big { T x; char c; };
			enum { diff = sizeof (Big) - sizeof (T), value = helper<diff>::template Val<Big>::value }; };
			template <typename T> struct EAAlignof3 { enum { x = EAAlignOf2<T>::value, y = EAAlignOf1<T>::value, value = x < y ? x : y }; };
			#define TURBO_ALIGN_OF(type) ((size_t)EAAlignof3<type>::value)

		#else
			// C implementation of TURBO_ALIGN_OF
			// This implementation works for most cases, but doesn't directly work
			// for types such as function pointer declarations. To work with those
			// types you need to typedef the type and then use the typedef in TURBO_ALIGN_OF.
			#define TURBO_ALIGN_OF(type) ((size_t)offsetof(struct { char c; type m; }, m))
		#endif
	#endif

	// TURBO_PRAGMA_PACK_VC
	//
	// Wraps #pragma pack in a way that allows for cleaner code.
	//
	// Example usage:
	//    TURBO_PRAGMA_PACK_VC(push, 1)
	//    struct X{ char c; int i; };
	//    TURBO_PRAGMA_PACK_VC(pop)
	//
	#if !defined(TURBO_PRAGMA_PACK_VC)
		#if defined(TURBO_COMPILER_MSVC)
			#define TURBO_PRAGMA_PACK_VC(...) __pragma(pack(__VA_ARGS__))
		#elif !defined(TURBO_COMPILER_NO_VARIADIC_MACROS)
			#define TURBO_PRAGMA_PACK_VC(...)
		#else
			// No support. However, all compilers of significance to us support variadic macros.
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_LIKELY / TURBO_UNLIKELY
	//
	// Defined as a macro which gives a hint to the compiler for branch
	// prediction. GCC gives you the ability to manually give a hint to
	// the compiler about the result of a comparison, though it's often
	// best to compile shipping code with profiling feedback under both
	// GCC (-fprofile-arcs) and VC++ (/LTCG:PGO, etc.). However, there
	// are times when you feel very sure that a boolean expression will
	// usually evaluate to either true or false and can help the compiler
	// by using an explicity directive...
	//
	// Example usage:
	//    if(TURBO_LIKELY(a == 0)) // Tell the compiler that a will usually equal 0.
	//       { ... }
	//
	// Example usage:
	//    if(TURBO_UNLIKELY(a == 0)) // Tell the compiler that a will usually not equal 0.
	//       { ... }
	//
	#ifndef TURBO_LIKELY
		#if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
			#if defined(__cplusplus)
				#define TURBO_LIKELY(x)   __builtin_expect(!!(x), true)
				#define TURBO_UNLIKELY(x) __builtin_expect(!!(x), false)
			#else
				#define TURBO_LIKELY(x)   __builtin_expect(!!(x), 1)
				#define TURBO_UNLIKELY(x) __builtin_expect(!!(x), 0)
			#endif
		#else
			#define TURBO_LIKELY(x)   (x)
			#define TURBO_UNLIKELY(x) (x)
		#endif
	#endif

	// ------------------------------------------------------------------------
	// TURBO_HAS_INCLUDE_AVAILABLE
	//
	// Used to guard against the TURBO_HAS_INCLUDE() macro on compilers that do not
	// support said feature.
	//
	// Example usage:
	//
	// #if TURBO_HAS_INCLUDE_AVAILABLE
	//     #if TURBO_HAS_INCLUDE("myinclude.h")
    //         #include "myinclude.h"
	//     #endif
	// #endif
	#if !defined(TURBO_HAS_INCLUDE_AVAILABLE)
		#if defined(TURBO_COMPILER_CPP17_ENABLED) || defined(TURBO_COMPILER_CLANG) || defined(TURBO_COMPILER_GNUC)
			#define TURBO_HAS_INCLUDE_AVAILABLE 1
		#else
			#define TURBO_HAS_INCLUDE_AVAILABLE 0
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_HAS_INCLUDE
	//
	// May be used in #if and #elif expressions to test for the existence
	// of the header referenced in the operand. If possible it evaluates to a
	// non-zero value and zero otherwise. The operand is the same form as the file
	// in a #include directive.
	//
	// Example usage:
	//
	// #if TURBO_HAS_INCLUDE("myinclude.h")
	//     #include "myinclude.h"
	// #endif
	//
	// #if TURBO_HAS_INCLUDE(<myinclude.h>)
	//     #include <myinclude.h>
	// #endif

	#if !defined(TURBO_HAS_INCLUDE)
		#if defined(TURBO_COMPILER_CPP17_ENABLED)
			#define TURBO_HAS_INCLUDE(x) __has_include(x)
		#elif defined(TURBO_COMPILER_CLANG)
			#define TURBO_HAS_INCLUDE(x) __has_include(x)
		#elif TURBO_COMPILER_GNUC
			#define TURBO_HAS_INCLUDE(x) __has_include(x)
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_INIT_PRIORITY_AVAILABLE
	//
	// This value is either not defined, or defined to 1.
	// Defines if the GCC attribute init_priority is supported by the compiler.
	//
	#if !defined(TURBO_INIT_PRIORITY_AVAILABLE)
		#if defined(__GNUC__) && !defined(__EDG__) // EDG typically #defines __GNUC__ but doesn't implement init_priority.
			#define TURBO_INIT_PRIORITY_AVAILABLE 1
		#elif defined(__clang__)
			#define TURBO_INIT_PRIORITY_AVAILABLE 1  // Clang implements init_priority
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_INIT_PRIORITY
	//
	// This is simply a wrapper for the GCC init_priority attribute that allows
	// multiplatform code to be easier to read. This attribute doesn't apply
	// to VC++ because VC++ uses file-level pragmas to control init ordering.
	//
	// Example usage:
	//     SomeClass gSomeClass TURBO_INIT_PRIORITY(2000);
	//
	#if !defined(TURBO_INIT_PRIORITY)
		#if defined(TURBO_INIT_PRIORITY_AVAILABLE)
			#define TURBO_INIT_PRIORITY(x)  __attribute__ ((init_priority (x)))
		#else
			#define TURBO_INIT_PRIORITY(x)
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_INIT_SEG_AVAILABLE
	//
	//
	#if !defined(TURBO_INIT_SEG_AVAILABLE)
		#if defined(_MSC_VER)
			#define TURBO_INIT_SEG_AVAILABLE 1
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_INIT_SEG
	//
	// Specifies a keyword or code section that affects the order in which startup code is executed.
	//
	// https://docs.microsoft.com/en-us/cpp/preprocessor/init-seg?view=vs-2019
	//
	// Example:
	// 		TURBO_INIT_SEG(compiler) MyType gMyTypeGlobal;
	// 		TURBO_INIT_SEG("my_section") MyOtherType gMyOtherTypeGlobal;
	//
	#if !defined(TURBO_INIT_SEG)
		#if defined(TURBO_INIT_SEG_AVAILABLE)
			#define TURBO_INIT_SEG(x)                                                                                                \
				__pragma(warning(push)) __pragma(warning(disable : 4074)) __pragma(warning(disable : 4075)) __pragma(init_seg(x)) \
					__pragma(warning(pop))
		#else
			#define TURBO_INIT_SEG(x)
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_MAY_ALIAS_AVAILABLE
	//
	// Defined as 0, 1, or 2.
	// Defines if the GCC attribute may_alias is supported by the compiler.
	// Consists of a value 0 (unsupported, shouldn't be used), 1 (some support),
	// or 2 (full proper support).
	//
	#ifndef TURBO_MAY_ALIAS_AVAILABLE
		#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 303)
			#if   !defined(__EDG__)                 // define it as 1 while defining GCC's support as 2.
				#define TURBO_MAY_ALIAS_AVAILABLE 2
			#else
				#define TURBO_MAY_ALIAS_AVAILABLE 0
			#endif
		#else
			#define TURBO_MAY_ALIAS_AVAILABLE 0
		#endif
	#endif


	// TURBO_MAY_ALIAS
	//
	// Defined as a macro that wraps the GCC may_alias attribute. This attribute
	// has no significance for VC++ because VC++ doesn't support the concept of
	// strict aliasing. Users should avoid writing code that breaks strict
	// aliasing rules; TURBO_MAY_ALIAS is for cases with no alternative.
	//
	// Example usage:
	//    void* TURBO_MAY_ALIAS gPtr = NULL;
	//
	// Example usage:
	//    typedef void* TURBO_MAY_ALIAS pvoid_may_alias;
	//    pvoid_may_alias gPtr = NULL;
	//
	#if TURBO_MAY_ALIAS_AVAILABLE
		#define TURBO_MAY_ALIAS __attribute__((__may_alias__))
	#else
		#define TURBO_MAY_ALIAS
	#endif


    // TURBO_HAVE_BUILTIN()
    //
    // Checks whether the compiler supports a Clang Feature Checking Macro, and if
    // so, checks whether it supports the provided builtin function "x" where x
    // is one of the functions noted in
    // https://clang.llvm.org/docs/LanguageExtensions.html
    //
    // Note: Use this macro to avoid an extra level of #ifdef __has_builtin check.
    // http://releases.llvm.org/3.3/tools/clang/docs/LanguageExtensions.html
    #ifdef __has_builtin
    #define TURBO_HAVE_BUILTIN(x) __has_builtin(x)
    #else
    #define TURBO_HAVE_BUILTIN(x) 0
    #endif

    // TURBO_ASSUME(cond)
    //
    // Informs the compiler that a condition is always true and that it can assume
    // it to be true for optimization purposes.
    //
    // WARNING: If the condition is false, the program can produce undefined and
    // potentially dangerous behavior.
    //
    // In !NDEBUG mode, the condition is checked with an assert().
    //
    // NOTE: The expression must not have side effects, as it may only be evaluated
    // in some compilation modes and not others. Some compilers may issue a warning
    // if the compiler cannot prove the expression has no side effects. For example,
    // the expression should not use a function call since the compiler cannot prove
    // that a function call does not have side effects.
    //
    // Example:
    //
    //   int x = ...;
    //   TURBO_ASSUME(x >= 0);
    //   // The compiler can optimize the division to a simple right shift using the
    //   // assumption specified above.
    //   int y = x / 16;
    //
    #if !defined(NDEBUG)
    #define TURBO_ASSUME(cond) assert(cond)
    #elif TURBO_HAVE_BUILTIN(__builtin_assume)
    #define TURBO_ASSUME(cond) __builtin_assume(cond)
    #elif defined(_MSC_VER)
    #define TURBO_ASSUME(cond) __assume(cond)
    #elif defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
    #define TURBO_ASSUME(cond)            \
      do {                               \
        if (!(cond)) std::unreachable(); \
      } while (false)
    #elif defined(__GNUC__) || TURBO_HAVE_BUILTIN(__builtin_unreachable)
    #define TURBO_ASSUME(cond)                 \
      do {                                    \
        if (!(cond)) __builtin_unreachable(); \
      } while (false)
    #else
    #define TURBO_ASSUME(cond)               \
      do {                                  \
        static_cast<void>(false && (cond)); \
      } while (false)
    #endif




// ------------------------------------------------------------------------
	// TURBO_ANALYSIS_ASSUME
	//
	// This acts the same as the VC++ __analysis_assume directive and is implemented
	// simply as a wrapper around it to allow portable usage of it and to take
	// advantage of it if and when it appears in other compilers.
	//
	// Example usage:
	//    char Function(char* p) {
	//       TURBO_ANALYSIS_ASSUME(p != NULL);
	//       return *p;
	//    }
	//
	#ifndef TURBO_ANALYSIS_ASSUME
		#if defined(_MSC_VER) && (_MSC_VER >= 1300) // If VC7.0 and later
			#define TURBO_ANALYSIS_ASSUME(x) __analysis_assume(!!(x)) // !! because that allows for convertible-to-bool in addition to bool.
		#else
			#define TURBO_ANALYSIS_ASSUME(x)
		#endif
	#endif



	// ------------------------------------------------------------------------
	// TURBO_DISABLE_VC_WARNING / TURBO_RESTORE_VC_WARNING
	//
	// Disable and re-enable warning(s) within code.
	// This is simply a wrapper for VC++ #pragma warning(disable: nnnn) for the
	// purpose of making code easier to read due to avoiding nested compiler ifdefs
	// directly in code.
	//
	// Example usage:
	//     TURBO_DISABLE_VC_WARNING(4127 3244)
	//     <code>
	//     TURBO_RESTORE_VC_WARNING()
	//
	#ifndef TURBO_DISABLE_VC_WARNING
		#if defined(_MSC_VER)
			#define TURBO_DISABLE_VC_WARNING(w)  \
				__pragma(warning(push))       \
				__pragma(warning(disable:w))
		#else
			#define TURBO_DISABLE_VC_WARNING(w)
		#endif
	#endif

	#ifndef TURBO_RESTORE_VC_WARNING
		#if defined(_MSC_VER)
			#define TURBO_RESTORE_VC_WARNING()   \
				__pragma(warning(pop))
		#else
			#define TURBO_RESTORE_VC_WARNING()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_ENABLE_VC_WARNING_AS_ERROR / TURBO_DISABLE_VC_WARNING_AS_ERROR
	//
	// Disable and re-enable treating a warning as error within code.
	// This is simply a wrapper for VC++ #pragma warning(error: nnnn) for the
	// purpose of making code easier to read due to avoiding nested compiler ifdefs
	// directly in code.
	//
	// Example usage:
	//     TURBO_ENABLE_VC_WARNING_AS_ERROR(4996)
	//     <code>
	//     TURBO_DISABLE_VC_WARNING_AS_ERROR()
	//
	#ifndef TURBO_ENABLE_VC_WARNING_AS_ERROR
		#if defined(_MSC_VER)
			#define TURBO_ENABLE_VC_WARNING_AS_ERROR(w) \
					__pragma(warning(push)) \
					__pragma(warning(error:w))
		#else
			#define TURBO_ENABLE_VC_WARNING_AS_ERROR(w)
		#endif
	#endif

	#ifndef TURBO_DISABLE_VC_WARNING_AS_ERROR
		#if defined(_MSC_VER)
			#define TURBO_DISABLE_VC_WARNING_AS_ERROR() \
				__pragma(warning(pop))
		#else
			#define TURBO_DISABLE_VC_WARNING_AS_ERROR()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_GCC_WARNING / TURBO_RESTORE_GCC_WARNING
	//
	// Example usage:
	//     // Only one warning can be ignored per statement, due to how GCC works.
	//     TURBO_DISABLE_GCC_WARNING(-Wuninitialized)
	//     TURBO_DISABLE_GCC_WARNING(-Wunused)
	//     <code>
	//     TURBO_RESTORE_GCC_WARNING()
	//     TURBO_RESTORE_GCC_WARNING()
	//
	#ifndef TURBO_DISABLE_GCC_WARNING
		#if defined(TURBO_COMPILER_GNUC)
			#define EAGCCWHELP0(x) #x
			#define EAGCCWHELP1(x) EAGCCWHELP0(GCC diagnostic ignored x)
			#define EAGCCWHELP2(x) EAGCCWHELP1(#x)
		#endif

		#if defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4006) // Can't test directly for __GNUC__ because some compilers lie.
			#define TURBO_DISABLE_GCC_WARNING(w)   \
				_Pragma("GCC diagnostic push")  \
				_Pragma(EAGCCWHELP2(w))
		#elif defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4004)
			#define TURBO_DISABLE_GCC_WARNING(w)   \
				_Pragma(EAGCCWHELP2(w))
		#else
			#define TURBO_DISABLE_GCC_WARNING(w)
		#endif
	#endif

	#ifndef TURBO_RESTORE_GCC_WARNING
		#if defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4006)
			#define TURBO_RESTORE_GCC_WARNING()    \
				_Pragma("GCC diagnostic pop")
		#else
			#define TURBO_RESTORE_GCC_WARNING()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_ALL_GCC_WARNINGS / TURBO_RESTORE_ALL_GCC_WARNINGS
	//
	// This isn't possible except via using _Pragma("GCC system_header"), though
	// that has some limitations in how it works. Another means is to manually
	// disable individual warnings within a GCC diagnostic push statement.
	// GCC doesn't have as many warnings as VC++ and EDG and so this may be feasible.
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// TURBO_ENABLE_GCC_WARNING_AS_ERROR / TURBO_DISABLE_GCC_WARNING_AS_ERROR
	//
	// Example usage:
	//     // Only one warning can be treated as an error per statement, due to how GCC works.
	//     TURBO_ENABLE_GCC_WARNING_AS_ERROR(-Wuninitialized)
	//     TURBO_ENABLE_GCC_WARNING_AS_ERROR(-Wunused)
	//     <code>
	//     TURBO_DISABLE_GCC_WARNING_AS_ERROR()
	//     TURBO_DISABLE_GCC_WARNING_AS_ERROR()
	//
	#ifndef TURBO_ENABLE_GCC_WARNING_AS_ERROR
		#if defined(TURBO_COMPILER_GNUC)
			#define EAGCCWERRORHELP0(x) #x
			#define EAGCCWERRORHELP1(x) EAGCCWERRORHELP0(GCC diagnostic error x)
			#define EAGCCWERRORHELP2(x) EAGCCWERRORHELP1(#x)
		#endif

		#if defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4006) // Can't test directly for __GNUC__ because some compilers lie.
			#define TURBO_ENABLE_GCC_WARNING_AS_ERROR(w)   \
				_Pragma("GCC diagnostic push")  \
				_Pragma(EAGCCWERRORHELP2(w))
		#elif defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4004)
			#define TURBO_DISABLE_GCC_WARNING(w)   \
				_Pragma(EAGCCWERRORHELP2(w))
		#else
			#define TURBO_DISABLE_GCC_WARNING(w)
		#endif
	#endif

	#ifndef TURBO_DISABLE_GCC_WARNING_AS_ERROR
		#if defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION >= 4006)
			#define TURBO_DISABLE_GCC_WARNING_AS_ERROR()    \
				_Pragma("GCC diagnostic pop")
		#else
			#define TURBO_DISABLE_GCC_WARNING_AS_ERROR()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_CLANG_WARNING / TURBO_RESTORE_CLANG_WARNING
	//
	// Example usage:
	//     // Only one warning can be ignored per statement, due to how clang works.
	//     TURBO_DISABLE_CLANG_WARNING(-Wuninitialized)
	//     TURBO_DISABLE_CLANG_WARNING(-Wunused)
	//     <code>
	//     TURBO_RESTORE_CLANG_WARNING()
	//     TURBO_RESTORE_CLANG_WARNING()
	//
	#ifndef TURBO_DISABLE_CLANG_WARNING
		#if defined(TURBO_COMPILER_CLANG) || defined(TURBO_COMPILER_CLANG_CL)
			#define EACLANGWHELP0(x) #x
			#define EACLANGWHELP1(x) EACLANGWHELP0(clang diagnostic ignored x)
			#define EACLANGWHELP2(x) EACLANGWHELP1(#x)

			#define TURBO_DISABLE_CLANG_WARNING(w)   \
				_Pragma("clang diagnostic push")  \
				_Pragma(EACLANGWHELP2(-Wunknown-warning-option))\
				_Pragma(EACLANGWHELP2(w))
		#else
			#define TURBO_DISABLE_CLANG_WARNING(w)
		#endif
	#endif

	#ifndef TURBO_RESTORE_CLANG_WARNING
		#if defined(TURBO_COMPILER_CLANG) || defined(TURBO_COMPILER_CLANG_CL)
			#define TURBO_RESTORE_CLANG_WARNING()    \
				_Pragma("clang diagnostic pop")
		#else
			#define TURBO_RESTORE_CLANG_WARNING()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_ALL_CLANG_WARNINGS / TURBO_RESTORE_ALL_CLANG_WARNINGS
	//
	// The situation for clang is the same as for GCC. See above.
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// TURBO_ENABLE_CLANG_WARNING_AS_ERROR / TURBO_DISABLE_CLANG_WARNING_AS_ERROR
	//
	// Example usage:
	//     // Only one warning can be treated as an error per statement, due to how clang works.
	//     TURBO_ENABLE_CLANG_WARNING_AS_ERROR(-Wuninitialized)
	//     TURBO_ENABLE_CLANG_WARNING_AS_ERROR(-Wunused)
	//     <code>
	//     TURBO_DISABLE_CLANG_WARNING_AS_ERROR()
	//     TURBO_DISABLE_CLANG_WARNING_AS_ERROR()
	//
	#ifndef TURBO_ENABLE_CLANG_WARNING_AS_ERROR
		#if defined(TURBO_COMPILER_CLANG) || defined(TURBO_COMPILER_CLANG_CL)
			#define EACLANGWERRORHELP0(x) #x
			#define EACLANGWERRORHELP1(x) EACLANGWERRORHELP0(clang diagnostic error x)
			#define EACLANGWERRORHELP2(x) EACLANGWERRORHELP1(#x)

			#define TURBO_ENABLE_CLANG_WARNING_AS_ERROR(w)   \
				_Pragma("clang diagnostic push")  \
				_Pragma(EACLANGWERRORHELP2(w))
		#else
			#define TURBO_DISABLE_CLANG_WARNING(w)
		#endif
	#endif

	#ifndef TURBO_DISABLE_CLANG_WARNING_AS_ERROR
		#if defined(TURBO_COMPILER_CLANG) || defined(TURBO_COMPILER_CLANG_CL)
			#define TURBO_DISABLE_CLANG_WARNING_AS_ERROR()    \
				_Pragma("clang diagnostic pop")
		#else
			#define TURBO_DISABLE_CLANG_WARNING_AS_ERROR()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_SN_WARNING / TURBO_RESTORE_SN_WARNING
	//
	// Note that we define this macro specifically for the SN compiler instead of
	// having a generic one for EDG-based compilers. The reason for this is that
	// while SN is indeed based on EDG, SN has different warning value mappings
	// and thus warning 1234 for SN is not the same as 1234 for all other EDG compilers.
	//
	// Example usage:
	//     // Currently we are limited to one warning per line.
	//     TURBO_DISABLE_SN_WARNING(1787)
	//     TURBO_DISABLE_SN_WARNING(552)
	//     <code>
	//     TURBO_RESTORE_SN_WARNING()
	//     TURBO_RESTORE_SN_WARNING()
	//
	#ifndef TURBO_DISABLE_SN_WARNING
			#define TURBO_DISABLE_SN_WARNING(w)
	#endif

	#ifndef TURBO_RESTORE_SN_WARNING
			#define TURBO_RESTORE_SN_WARNING()
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_ALL_SN_WARNINGS / TURBO_RESTORE_ALL_SN_WARNINGS
	//
	// Example usage:
	//     TURBO_DISABLE_ALL_SN_WARNINGS()
	//     <code>
	//     TURBO_RESTORE_ALL_SN_WARNINGS()
	//
	#ifndef TURBO_DISABLE_ALL_SN_WARNINGS
			#define TURBO_DISABLE_ALL_SN_WARNINGS()
	#endif

	#ifndef TURBO_RESTORE_ALL_SN_WARNINGS
			#define TURBO_RESTORE_ALL_SN_WARNINGS()
	#endif



	// ------------------------------------------------------------------------
	// TURBO_DISABLE_GHS_WARNING / TURBO_RESTORE_GHS_WARNING
	//
	// Disable warnings from the Green Hills compiler.
	//
	// Example usage:
	//     TURBO_DISABLE_GHS_WARNING(193)
	//     TURBO_DISABLE_GHS_WARNING(236, 5323)
	//     <code>
	//     TURBO_RESTORE_GHS_WARNING()
	//     TURBO_RESTORE_GHS_WARNING()
	//
	#ifndef TURBO_DISABLE_GHS_WARNING
			#define TURBO_DISABLE_GHS_WARNING(w)
	#endif

	#ifndef TURBO_RESTORE_GHS_WARNING
			#define TURBO_RESTORE_GHS_WARNING()
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_ALL_GHS_WARNINGS / TURBO_RESTORE_ALL_GHS_WARNINGS
	//
	// #ifndef TURBO_DISABLE_ALL_GHS_WARNINGS
	//     #if defined(TURBO_COMPILER_GREEN_HILLS)
	//         #define TURBO_DISABLE_ALL_GHS_WARNINGS(w)  \_
	//             _Pragma("_________")
	//     #else
	//         #define TURBO_DISABLE_ALL_GHS_WARNINGS(w)
	//     #endif
	// #endif
	//
	// #ifndef TURBO_RESTORE_ALL_GHS_WARNINGS
	//     #if defined(TURBO_COMPILER_GREEN_HILLS)
	//         #define TURBO_RESTORE_ALL_GHS_WARNINGS()   \_
	//             _Pragma("_________")
	//     #else
	//         #define TURBO_RESTORE_ALL_GHS_WARNINGS()
	//     #endif
	// #endif



	// ------------------------------------------------------------------------
	// TURBO_DISABLE_EDG_WARNING / TURBO_RESTORE_EDG_WARNING
	//
	// Example usage:
	//     // Currently we are limited to one warning per line.
	//     TURBO_DISABLE_EDG_WARNING(193)
	//     TURBO_DISABLE_EDG_WARNING(236)
	//     <code>
	//     TURBO_RESTORE_EDG_WARNING()
	//     TURBO_RESTORE_EDG_WARNING()
	//
	#ifndef TURBO_DISABLE_EDG_WARNING
		// EDG-based compilers are inconsistent in how the implement warning pragmas.
		#if defined(TURBO_COMPILER_EDG) && !defined(TURBO_COMPILER_INTEL) && !defined(TURBO_COMPILER_RVCT)
			#define EAEDGWHELP0(x) #x
			#define EAEDGWHELP1(x) EAEDGWHELP0(diag_suppress x)

			#define TURBO_DISABLE_EDG_WARNING(w)   \
				_Pragma("control %push diag")   \
				_Pragma(EAEDGWHELP1(w))
		#else
			#define TURBO_DISABLE_EDG_WARNING(w)
		#endif
	#endif

	#ifndef TURBO_RESTORE_EDG_WARNING
		#if defined(TURBO_COMPILER_EDG) && !defined(TURBO_COMPILER_INTEL) && !defined(TURBO_COMPILER_RVCT)
			#define TURBO_RESTORE_EDG_WARNING()   \
				_Pragma("control %pop diag")
		#else
			#define TURBO_RESTORE_EDG_WARNING()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_ALL_EDG_WARNINGS / TURBO_RESTORE_ALL_EDG_WARNINGS
	//
	//#ifndef TURBO_DISABLE_ALL_EDG_WARNINGS
	//    #if defined(TURBO_COMPILER_EDG) && !defined(TURBO_COMPILER_SN)
	//        #define TURBO_DISABLE_ALL_EDG_WARNINGS(w)  \_
	//            _Pragma("_________")
	//    #else
	//        #define TURBO_DISABLE_ALL_EDG_WARNINGS(w)
	//    #endif
	//#endif
	//
	//#ifndef TURBO_RESTORE_ALL_EDG_WARNINGS
	//    #if defined(TURBO_COMPILER_EDG) && !defined(TURBO_COMPILER_SN)
	//        #define TURBO_RESTORE_ALL_EDG_WARNINGS()   \_
	//            _Pragma("_________")
	//    #else
	//        #define TURBO_RESTORE_ALL_EDG_WARNINGS()
	//    #endif
	//#endif



	// ------------------------------------------------------------------------
	// TURBO_DISABLE_CW_WARNING / TURBO_RESTORE_CW_WARNING
	//
	// Note that this macro can only control warnings via numbers and not by
	// names. The reason for this is that the compiler's syntax for such
	// warnings is not the same as for numbers.
	//
	// Example usage:
	//     // Currently we are limited to one warning per line and must also specify the warning in the restore macro.
	//     TURBO_DISABLE_CW_WARNING(10317)
	//     TURBO_DISABLE_CW_WARNING(10324)
	//     <code>
	//     TURBO_RESTORE_CW_WARNING(10317)
	//     TURBO_RESTORE_CW_WARNING(10324)
	//
	#ifndef TURBO_DISABLE_CW_WARNING
		#define TURBO_DISABLE_CW_WARNING(w)
	#endif

	#ifndef TURBO_RESTORE_CW_WARNING

		#define TURBO_RESTORE_CW_WARNING(w)

	#endif


	// ------------------------------------------------------------------------
	// TURBO_DISABLE_ALL_CW_WARNINGS / TURBO_RESTORE_ALL_CW_WARNINGS
	//
	#ifndef TURBO_DISABLE_ALL_CW_WARNINGS
		#define TURBO_DISABLE_ALL_CW_WARNINGS()

	#endif

	#ifndef TURBO_RESTORE_ALL_CW_WARNINGS
		#define TURBO_RESTORE_ALL_CW_WARNINGS()
	#endif



	// ------------------------------------------------------------------------
	// TURBO_PURE
	//
	// This acts the same as the GCC __attribute__ ((pure)) directive and is
	// implemented simply as a wrapper around it to allow portable usage of
	// it and to take advantage of it if and when it appears in other compilers.
	//
	// A "pure" function is one that has no effects except its return value and
	// its return value is a function of only the function's parameters or
	// non-volatile global variables. Any parameter or global variable access
	// must be read-only. Loop optimization and subexpression elimination can be
	// applied to such functions. A common example is strlen(): Given identical
	// inputs, the function's return value (its only effect) is invariant across
	// multiple invocations and thus can be pulled out of a loop and called but once.
	//
	// Example usage:
	//    TURBO_PURE void Function();
	//
	#ifndef TURBO_PURE
		#if defined(TURBO_COMPILER_GNUC)
			#define TURBO_PURE __attribute__((pure))
		#elif defined(TURBO_COMPILER_ARM)  // Arm brand compiler for ARM CPU
			#define TURBO_PURE __pure
		#else
			#define TURBO_PURE
		#endif
	#endif



	// ------------------------------------------------------------------------
	// TURBO_WEAK
	// TURBO_WEAK_SUPPORTED -- defined as 0 or 1.
	//
	// GCC
	// The weak attribute causes the declaration to be emitted as a weak
	// symbol rather than a global. This is primarily useful in defining
	// library functions which can be overridden in user code, though it
	// can also be used with non-function declarations.
	//
	// VC++
	// At link time, if multiple definitions of a COMDAT are seen, the linker
	// picks one and discards the rest. If the linker option /OPT:REF
	// is selected, then COMDAT elimination will occur to remove all the
	// unreferenced data items in the linker output.
	//
	// Example usage:
	//    TURBO_WEAK void Function();
	//
	#ifndef TURBO_WEAK
		#if defined(_MSC_VER) && (_MSC_VER >= 1300) // If VC7.0 and later
			#define TURBO_WEAK __declspec(selectany)
			#define TURBO_WEAK_SUPPORTED 1
		#elif defined(_MSC_VER) || (defined(__GNUC__) && defined(__CYGWIN__))
			#define TURBO_WEAK
			#define TURBO_WEAK_SUPPORTED 0
		#elif defined(TURBO_COMPILER_ARM)  // Arm brand compiler for ARM CPU
			#define TURBO_WEAK __weak
			#define TURBO_WEAK_SUPPORTED 1
		#else                           // GCC and IBM compilers, others.
			#define TURBO_WEAK __attribute__((weak))
			#define TURBO_WEAK_SUPPORTED 1
		#endif
	#endif



	// ------------------------------------------------------------------------
	// TURBO_UNUSED
	//
	// Makes compiler warnings about unused variables go away.
	//
	// Example usage:
	//    void Function(int x)
	//    {
	//        int y;
	//        TURBO_UNUSED(x);
	//        TURBO_UNUSED(y);
	//    }
	//
	#ifndef TURBO_UNUSED
		// The EDG solution below is pretty weak and needs to be augmented or replaced.
		// It can't handle the C language, is limited to places where template declarations
		// can be used, and requires the type x to be usable as a functions reference argument.
		#if defined(__cplusplus) && defined(__EDG__)
			template <typename T>
			inline void TBBaseUnused(T const volatile & x) { (void)x; }
			#define TURBO_UNUSED(x) TBBaseUnused(x)
		#else
			#define TURBO_UNUSED(x) (void)x
		#endif
	#endif



	// ------------------------------------------------------------------------
	// TURBO_EMPTY
	//
	// Allows for a null statement, usually for the purpose of avoiding compiler warnings.
	//
	// Example usage:
	//    #ifdef TURBO_DEBUG
	//        #define MyDebugPrintf(x, y) printf(x, y)
	//    #else
	//        #define MyDebugPrintf(x, y)  TURBO_EMPTY
	//    #endif
	//
	#ifndef TURBO_EMPTY
		#define TURBO_EMPTY (void)0
	#endif


	// ------------------------------------------------------------------------
	// TURBO_CURRENT_FUNCTION
	//
	// Provides a consistent way to get the current function name as a macro
	// like the __FILE__ and __LINE__ macros work. The C99 standard specifies
	// that __func__ be provided by the compiler, but most compilers don't yet
	// follow that convention. However, many compilers have an alternative.
	//
	// We also define TURBO_CURRENT_FUNCTION_SUPPORTED for when it is not possible
	// to have TURBO_CURRENT_FUNCTION work as expected.
	//
	// Defined inside a function because otherwise the macro might not be
	// defined and code below might not compile. This happens with some
	// compilers.
	//
	#ifndef TURBO_CURRENT_FUNCTION
		#if defined __GNUC__ || (defined __ICC && __ICC >= 600)
			#define TURBO_CURRENT_FUNCTION __PRETTY_FUNCTION__
		#elif defined(__FUNCSIG__)
			#define TURBO_CURRENT_FUNCTION __FUNCSIG__
		#elif (defined __INTEL_COMPILER && __INTEL_COMPILER >= 600) || (defined __IBMCPP__ && __IBMCPP__ >= 500) || (defined CS_UNDEFINED_STRING && CS_UNDEFINED_STRING >= 0x4200)
			#define TURBO_CURRENT_FUNCTION __FUNCTION__
		#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901
			#define TURBO_CURRENT_FUNCTION __func__
		#else
			#define TURBO_CURRENT_FUNCTION "(unknown function)"
		#endif
	#endif


	// ------------------------------------------------------------------------
	// wchar_t
	// Here we define:
	//    TURBO_WCHAR_T_NON_NATIVE
	//    TURBO_WCHAR_SIZE = <sizeof(wchar_t)>
	//
	#ifndef TURBO_WCHAR_T_NON_NATIVE
		// Compilers that always implement wchar_t as native include:
		//     COMEAU, new SN, and other EDG-based compilers.
		//     GCC
		//     Borland
		//     SunPro
		//     IBM Visual Age
		#if defined(TURBO_COMPILER_INTEL)
			#if (TURBO_COMPILER_VERSION < 700)
				#define TURBO_WCHAR_T_NON_NATIVE 1
			#else
				#if (!defined(_WCHAR_T_DEFINED) && !defined(_WCHAR_T))
					#define TURBO_WCHAR_T_NON_NATIVE 1
				#endif
			#endif
		#elif defined(TURBO_COMPILER_MSVC) || (defined(TURBO_COMPILER_CLANG) && defined(TURBO_PLATFORM_WINDOWS))
			#ifndef _NATIVE_WCHAR_T_DEFINED
				#define TURBO_WCHAR_T_NON_NATIVE 1
			#endif
		#elif defined(__EDG_VERSION__) && (!defined(_WCHAR_T) && (__EDG_VERSION__ < 400)) // EDG prior to v4 uses _WCHAR_T to indicate if wchar_t is native. v4+ may define something else, but we're not currently aware of it.
			#define TURBO_WCHAR_T_NON_NATIVE 1
		#endif
	#endif

	#ifndef TURBO_WCHAR_SIZE // If the user hasn't specified that it is a given size...
		#if defined(__WCHAR_MAX__) // GCC defines this for most platforms.
			#if (__WCHAR_MAX__ == 2147483647) || (__WCHAR_MAX__ == 4294967295)
				#define TURBO_WCHAR_SIZE 4
			#elif (__WCHAR_MAX__ == 32767) || (__WCHAR_MAX__ == 65535)
				#define TURBO_WCHAR_SIZE 2
			#elif (__WCHAR_MAX__ == 127) || (__WCHAR_MAX__ == 255)
				#define TURBO_WCHAR_SIZE 1
			#else
				#define TURBO_WCHAR_SIZE 4
			#endif
		#elif defined(WCHAR_MAX) // The SN and Arm compilers define this.
			#if (WCHAR_MAX == 2147483647) || (WCHAR_MAX == 4294967295)
				#define TURBO_WCHAR_SIZE 4
			#elif (WCHAR_MAX == 32767) || (WCHAR_MAX == 65535)
				#define TURBO_WCHAR_SIZE 2
			#elif (WCHAR_MAX == 127) || (WCHAR_MAX == 255)
				#define TURBO_WCHAR_SIZE 1
			#else
				#define TURBO_WCHAR_SIZE 4
			#endif
		#elif defined(__WCHAR_BIT) // Green Hills (and other versions of EDG?) uses this.
			#if (__WCHAR_BIT == 16)
				#define TURBO_WCHAR_SIZE 2
			#elif (__WCHAR_BIT == 32)
				#define TURBO_WCHAR_SIZE 4
			#elif (__WCHAR_BIT == 8)
				#define TURBO_WCHAR_SIZE 1
			#else
				#define TURBO_WCHAR_SIZE 4
			#endif
		#elif defined(_WCMAX) // The SN and Arm compilers define this.
			#if (_WCMAX == 2147483647) || (_WCMAX == 4294967295)
				#define TURBO_WCHAR_SIZE 4
			#elif (_WCMAX == 32767) || (_WCMAX == 65535)
				#define TURBO_WCHAR_SIZE 2
			#elif (_WCMAX == 127) || (_WCMAX == 255)
				#define TURBO_WCHAR_SIZE 1
			#else
				#define TURBO_WCHAR_SIZE 4
			#endif
		#elif defined(TURBO_PLATFORM_UNIX)
			// It is standard on Unix to have wchar_t be int32_t or uint32_t.
			// All versions of GNUC default to a 32 bit wchar_t, but EA has used
			// the -fshort-wchar GCC command line option to force it to 16 bit.
			// If you know that the compiler is set to use a wchar_t of other than
			// the default, you need to manually define TURBO_WCHAR_SIZE for the build.
			#define TURBO_WCHAR_SIZE 4
		#else
			// It is standard on Windows to have wchar_t be uint16_t.  GCC
			// defines wchar_t as int by default.  Electronic Arts has
			// standardized on wchar_t being an unsigned 16 bit value on all
			// console platforms. Given that there is currently no known way to
			// tell at preprocessor time what the size of wchar_t is, we declare
			// it to be 2, as this is the Electronic Arts standard. If you have
			// TURBO_WCHAR_SIZE != sizeof(wchar_t), then your code might not be
			// broken, but it also won't work with wchar libraries and data from
			// other parts of EA. Under GCC, you can force wchar_t to two bytes
			// with the -fshort-wchar compiler argument.
			#define TURBO_WCHAR_SIZE 2
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_RESTRICT
	//
	// The C99 standard defines a new keyword, restrict, which allows for the
	// improvement of code generation regarding memory usage. Compilers can
	// generate significantly faster code when you are able to use restrict.
	//
	// Example usage:
	//    void DoSomething(char* TURBO_RESTRICT p1, char* TURBO_RESTRICT p2);
	//
	#ifndef TURBO_RESTRICT
		#if defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
			#define TURBO_RESTRICT __restrict
		#elif defined(TURBO_COMPILER_CLANG)
			#define TURBO_RESTRICT __restrict
		#elif defined(TURBO_COMPILER_GNUC)     // Includes GCC and other compilers emulating GCC.
			#define TURBO_RESTRICT __restrict  // GCC defines 'restrict' (as opposed to __restrict) in C99 mode only.
		#elif defined(TURBO_COMPILER_ARM)
			#define TURBO_RESTRICT __restrict
		#elif defined(TURBO_COMPILER_IS_C99)
			#define TURBO_RESTRICT restrict
		#else
			// If the compiler didn't support restricted pointers, defining TURBO_RESTRICT
			// away would result in compiling and running fine but you just wouldn't
			// the same level of optimization. On the other hand, all the major compilers
			// support restricted pointers.
			#define TURBO_RESTRICT
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_DEPRECATED            // Used as a prefix.
	// TURBO_PREFIX_DEPRECATED     // You should need this only for unusual compilers.
	// TURBO_POSTFIX_DEPRECATED    // You should need this only for unusual compilers.
	// TURBO_DEPRECATED_MESSAGE    // Used as a prefix and provides a deprecation message.
	//
	// Example usage:
	//    TURBO_DEPRECATED void Function();
	//    TURBO_DEPRECATED_MESSAGE("Use 1.0v API instead") void Function();
	//
	// or for maximum portability:
	//    TURBO_PREFIX_DEPRECATED void Function() TURBO_POSTFIX_DEPRECATED;
	//

	#ifndef TURBO_DEPRECATED
		#if defined(TURBO_COMPILER_CPP14_ENABLED)
			#define TURBO_DEPRECATED [[deprecated]]
		#elif defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION > 1300) // If VC7 (VS2003) or later...
			#define TURBO_DEPRECATED __declspec(deprecated)
		#elif defined(TURBO_COMPILER_MSVC)
			#define TURBO_DEPRECATED
		#else
			#define TURBO_DEPRECATED __attribute__((deprecated))
		#endif
	#endif

	#ifndef TURBO_PREFIX_DEPRECATED
		#if defined(TURBO_COMPILER_CPP14_ENABLED)
			#define TURBO_PREFIX_DEPRECATED [[deprecated]]
			#define TURBO_POSTFIX_DEPRECATED
		#elif defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION > 1300) // If VC7 (VS2003) or later...
			#define TURBO_PREFIX_DEPRECATED __declspec(deprecated)
			#define TURBO_POSTFIX_DEPRECATED
		#elif defined(TURBO_COMPILER_MSVC)
			#define TURBO_PREFIX_DEPRECATED
			#define TURBO_POSTFIX_DEPRECATED
		#else
			#define TURBO_PREFIX_DEPRECATED
			#define TURBO_POSTFIX_DEPRECATED __attribute__((deprecated))
		#endif
	#endif

	#ifndef TURBO_DEPRECATED_MESSAGE
		#if defined(TURBO_COMPILER_CPP14_ENABLED)
			#define TURBO_DEPRECATED_MESSAGE(msg) [[deprecated(#msg)]]
        #elif TURBO_HAVE_ATTRIBUTE(deprecated)
            #define TURBO_DEPRECATED_MESSAGE(message) __attribute__((deprecated(message)))
		#else
			// Compiler does not support depreaction messages, explicitly drop the msg but still mark the function as deprecated
			#define TURBO_DEPRECATED_MESSAGE(msg) TURBO_DEPRECATED
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_FORCE_INLINE              // Used as a prefix.
	// TURBO_PREFIX_FORCE_INLINE       // You should need this only for unusual compilers.
	// TURBO_POSTFIX_FORCE_INLINE      // You should need this only for unusual compilers.
	//
	// Example usage:
	//     TURBO_FORCE_INLINE void Foo();                                // Implementation elsewhere.
	//     TURBO_PREFIX_FORCE_INLINE void Foo() TURBO_POSTFIX_FORCE_INLINE; // Implementation elsewhere.
	//
	// Note that when the prefix version of this function is used, it replaces
	// the regular C++ 'inline' statement. Thus you should not use both the
	// C++ inline statement and this macro with the same function declaration.
	//
	// To force inline usage under GCC 3.1+, you use this:
	//    inline void Foo() __attribute__((always_inline));
	//       or
	//    inline __attribute__((always_inline)) void Foo();
	//
	// The CodeWarrior compiler doesn't have the concept of forcing inlining per function.
	//
	#ifndef TURBO_FORCE_INLINE
		#if defined(TURBO_COMPILER_MSVC)
			#define TURBO_FORCE_INLINE __forceinline
		#elif defined(TURBO_COMPILER_GNUC) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 301) || defined(TURBO_COMPILER_CLANG)
			#if defined(__cplusplus)
				#define TURBO_FORCE_INLINE inline __attribute__((always_inline))
			#else
				#define TURBO_FORCE_INLINE __inline__ __attribute__((always_inline))
			#endif
		#else
			#if defined(__cplusplus)
				#define TURBO_FORCE_INLINE inline
			#else
				#define TURBO_FORCE_INLINE __inline
			#endif
		#endif
	#endif

	#if   defined(TURBO_COMPILER_GNUC) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 301) || defined(TURBO_COMPILER_CLANG)
		#define TURBO_PREFIX_FORCE_INLINE  inline
		#define TURBO_POSTFIX_FORCE_INLINE __attribute__((always_inline))
	#else
		#define TURBO_PREFIX_FORCE_INLINE  inline
		#define TURBO_POSTFIX_FORCE_INLINE
	#endif


	// ------------------------------------------------------------------------
	// TURBO_FORCE_INLINE_LAMBDA
	//
	// TURBO_FORCE_INLINE_LAMBDA is used to force inline a call to a lambda when possible.
	// Force inlining a lambda can be useful to reduce overhead in situations where a lambda may
	// may only be called once, or inlining allows the compiler to apply other optimizations that wouldn't
	// otherwise be possible.
	//
	// The ability to force inline a lambda is currently only available on a subset of compilers.
	//
	// Example usage:
	//
	//		auto lambdaFunction = []() TURBO_FORCE_INLINE_LAMBDA
	//		{
	//		};
	//
	#ifndef TURBO_FORCE_INLINE_LAMBDA
		#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
			#define TURBO_FORCE_INLINE_LAMBDA __attribute__((always_inline))
		#else
			#define TURBO_FORCE_INLINE_LAMBDA
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_NO_INLINE             // Used as a prefix.
	// TURBO_PREFIX_NO_INLINE      // You should need this only for unusual compilers.
	// TURBO_POSTFIX_NO_INLINE     // You should need this only for unusual compilers.
	//
	// Example usage:
	//     TURBO_NO_INLINE        void Foo();                       // Implementation elsewhere.
	//     TURBO_PREFIX_NO_INLINE void Foo() TURBO_POSTFIX_NO_INLINE;  // Implementation elsewhere.
	//
	// That this declaration is incompatbile with C++ 'inline' and any
	// variant of TURBO_FORCE_INLINE.
	//
	// To disable inline usage under VC++ priof to VS2005, you need to use this:
	//    #pragma inline_depth(0) // Disable inlining.
	//    void Foo() { ... }
	//    #pragma inline_depth()  // Restore to default.
	//
	// Since there is no easy way to disable inlining on a function-by-function
	// basis in VC++ prior to VS2005, the best strategy is to write platform-specific
	// #ifdefs in the code or to disable inlining for a given module and enable
	// functions individually with TURBO_FORCE_INLINE.
	//
	#ifndef TURBO_NO_INLINE
		#if defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
			#define TURBO_NO_INLINE __declspec(noinline)
		#elif defined(TURBO_COMPILER_MSVC)
			#define TURBO_NO_INLINE
		#else
			#define TURBO_NO_INLINE __attribute__((noinline))
		#endif
	#endif

	#if defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
		#define TURBO_PREFIX_NO_INLINE  __declspec(noinline)
		#define TURBO_POSTFIX_NO_INLINE
	#elif defined(TURBO_COMPILER_MSVC)
		#define TURBO_PREFIX_NO_INLINE
		#define TURBO_POSTFIX_NO_INLINE
	#else
		#define TURBO_PREFIX_NO_INLINE
		#define TURBO_POSTFIX_NO_INLINE __attribute__((noinline))
	#endif


	// ------------------------------------------------------------------------
	// TURBO_NO_VTABLE
	//
	// Example usage:
	//     class TURBO_NO_VTABLE X {
	//        virtual void InterfaceFunction();
	//     };
	//
	//     TURBO_CLASS_NO_VTABLE(X) {
	//        virtual void InterfaceFunction();
	//     };
	//
	#ifdef TURBO_COMPILER_MSVC
		#define TURBO_NO_VTABLE           __declspec(novtable)
		#define TURBO_CLASS_NO_VTABLE(x)  class __declspec(novtable) x
		#define TURBO_STRUCT_NO_VTABLE(x) struct __declspec(novtable) x
	#else
		#define TURBO_NO_VTABLE
		#define TURBO_CLASS_NO_VTABLE(x)  class x
		#define TURBO_STRUCT_NO_VTABLE(x) struct x
	#endif


	// ------------------------------------------------------------------------
	// TURBO_PASCAL
	//
	// Also known on PC platforms as stdcall.
	// This convention causes the compiler to assume that the called function
	// will pop off the stack space used to pass arguments, unless it takes a
	// variable number of arguments.
	//
	// Example usage:
	//    this:
	//       void DoNothing(int x);
	//       void DoNothing(int x){}
	//    would be written as this:
	//       void TURBO_PASCAL_FUNC(DoNothing(int x));
	//       void TURBO_PASCAL_FUNC(DoNothing(int x)){}
	//
	#ifndef TURBO_PASCAL
		#if defined(TURBO_COMPILER_MSVC)
			#define TURBO_PASCAL __stdcall
		#elif defined(TURBO_COMPILER_GNUC) && defined(TURBO_PROCESSOR_X86)
			#define TURBO_PASCAL __attribute__((stdcall))
		#else
			// Some compilers simply don't support pascal calling convention.
			// As a result, there isn't an issue here, since the specification of
			// pascal calling convention is for the purpose of disambiguating the
			// calling convention that is applied.
			#define TURBO_PASCAL
		#endif
	#endif

	#ifndef TURBO_PASCAL_FUNC
		#if defined(TURBO_COMPILER_MSVC)
			#define TURBO_PASCAL_FUNC(funcname_and_paramlist)    __stdcall funcname_and_paramlist
		#elif defined(TURBO_COMPILER_GNUC) && defined(TURBO_PROCESSOR_X86)
			#define TURBO_PASCAL_FUNC(funcname_and_paramlist)    __attribute__((stdcall)) funcname_and_paramlist
		#else
			#define TURBO_PASCAL_FUNC(funcname_and_paramlist)    funcname_and_paramlist
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_SSE
	// Visual C Processor Packs define _MSC_FULL_VER and are needed for SSE
	// Intel C also has SSE support.
	// TURBO_SSE is used to select FPU or SSE versions in hw_select.inl
	//
	// TURBO_SSE defines the level of SSE support:
	//  0 indicates no SSE support
	//  1 indicates SSE1 is supported
	//  2 indicates SSE2 is supported
	//  3 indicates SSE3 (or greater) is supported
	//
	// Note: SSE support beyond SSE3 can't be properly represented as a single
	// version number.  Instead users should use specific SSE defines (e.g.
	// TURBO_SSE4_2) to detect what specific support is available.  TURBO_SSE being
	// equal to 3 really only indicates that SSE3 or greater is supported.
	#ifndef TURBO_SSE
		#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
			#if defined(__SSE3__)
				#define TURBO_SSE 3
			#elif defined(__SSE2__)
				#define TURBO_SSE 2
			#elif defined(__SSE__) && __SSE__
				#define TURBO_SSE 1
			#else
				#define TURBO_SSE 0
			#endif
		#elif (defined(TURBO_SSE3) && TURBO_SSE3) || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define TURBO_SSE 3
		#elif defined(TURBO_SSE2) && TURBO_SSE2
			#define TURBO_SSE 2
		#elif defined(TURBO_PROCESSOR_X86) && defined(_MSC_FULL_VER) && !defined(__NOSSE__) && defined(_M_IX86_FP)
			#define TURBO_SSE _M_IX86_FP
		#elif defined(TURBO_PROCESSOR_X86) && defined(TURBO_COMPILER_INTEL) && !defined(__NOSSE__)
			#define TURBO_SSE 1
		#elif defined(TURBO_PROCESSOR_X86_64)
			// All x64 processors support SSE2 or higher
			#define TURBO_SSE 2
		#else
			#define TURBO_SSE 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// We define separate defines for SSE support beyond SSE1.  These defines
	// are particularly useful for detecting SSE4.x features since there isn't
	// a single concept of SSE4.
	//
	// The following SSE defines are always defined.  0 indicates the
	// feature/level of SSE is not supported, and 1 indicates support is
	// available.
	#ifndef TURBO_SSE2
		#if TURBO_SSE >= 2
			#define TURBO_SSE2 1
		#else
			#define TURBO_SSE2 0
		#endif
	#endif
	#ifndef TURBO_SSE3
		#if TURBO_SSE >= 3
			#define TURBO_SSE3 1
		#else
			#define TURBO_SSE3 0
		#endif
	#endif
	#ifndef TURBO_SSSE3
		#if defined __SSSE3__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define TURBO_SSSE3 1
		#else
			#define TURBO_SSSE3 0
		#endif
	#endif
	#ifndef TURBO_SSE4_1
		#if defined __SSE4_1__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define TURBO_SSE4_1 1
		#else
			#define TURBO_SSE4_1 0
		#endif
	#endif
	#ifndef TURBO_SSE4_2
		#if defined __SSE4_2__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define TURBO_SSE4_2 1
		#else
			#define TURBO_SSE4_2 0
		#endif
	#endif
	#ifndef TURBO_SSE4A
		#if defined __SSE4A__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define TURBO_SSE4A 1
		#else
			#define TURBO_SSE4A 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// TURBO_AVX
	// TURBO_AVX may be used to determine if Advanced Vector Extensions are available for the target architecture
	//
	// TURBO_AVX defines the level of AVX support:
	//  0 indicates no AVX support
	//  1 indicates AVX1 is supported
	//  2 indicates AVX2 is supported
	#ifndef TURBO_AVX
		#if defined __AVX2__
			#define TURBO_AVX 2
		#elif defined __AVX__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define TURBO_AVX 1
		#else
			#define TURBO_AVX 0
		#endif
	#endif
	#ifndef TURBO_AVX2
		#if TURBO_AVX >= 2
			#define TURBO_AVX2 1
		#else
			#define TURBO_AVX2 0
		#endif
	#endif

	// TURBO_FP16C may be used to determine the existence of float <-> half conversion operations on an x86 CPU.
	// (For example to determine if _mm_cvtph_ps or _mm_cvtps_ph could be used.)
	#ifndef TURBO_FP16C
		#if defined __F16C__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define TURBO_FP16C 1
		#else
			#define TURBO_FP16C 0
		#endif
	#endif

	// TURBO_FP128 may be used to determine if __float128 is a supported type for use. This type is enabled by a GCC extension (_GLIBCXX_USE_FLOAT128)
	// but has support by some implementations of clang (__FLOAT128__)
	// PS4 does not support __float128 as of SDK 5.500 https://ps4.siedev.net/resources/documents/SDK/5.500/CPU_Compiler_ABI-Overview/0003.html
	#ifndef TURBO_FP128
		#if (defined __FLOAT128__ || defined _GLIBCXX_USE_FLOAT128) && !defined(TURBO_PLATFORM_SONY)
			#define TURBO_FP128 1
		#else
			#define TURBO_FP128 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// TURBO_ABM
	// TURBO_ABM may be used to determine if Advanced Bit Manipulation sets are available for the target architecture (POPCNT, LZCNT)
	//
	#ifndef TURBO_ABM
		#if defined(__ABM__) || defined(TURBO_PLATFORM_XBOXONE) || defined(TURBO_PLATFORM_SONY) || defined(CS_UNDEFINED_STRING)
			#define TURBO_ABM 1
		#else
			#define TURBO_ABM 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// TURBO_NEON
	// TURBO_NEON may be used to determine if NEON is supported.
	#ifndef TURBO_NEON
		#if defined(__ARM_NEON__) || defined(__ARM_NEON)
			#define TURBO_NEON 1
		#else
			#define TURBO_NEON 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// TURBO_BMI
	// TURBO_BMI may be used to determine if Bit Manipulation Instruction sets are available for the target architecture
	//
	// TURBO_BMI defines the level of BMI support:
	//  0 indicates no BMI support
	//  1 indicates BMI1 is supported
	//  2 indicates BMI2 is supported
	#ifndef TURBO_BMI
		#if defined(__BMI2__)
			#define TURBO_BMI 2
		#elif defined(__BMI__) || defined(TURBO_PLATFORM_XBOXONE) || defined(CS_UNDEFINED_STRING)
			#define TURBO_BMI 1
		#else
			#define TURBO_BMI 0
		#endif
	#endif
	#ifndef TURBO_BMI2
		#if TURBO_BMI >= 2
			#define TURBO_BMI2 1
		#else
			#define TURBO_BMI2 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// TURBO_FMA3
	// TURBO_FMA3 may be used to determine if Fused Multiply Add operations are available for the target architecture
	// __FMA__ is defined only by GCC, Clang, and ICC; MSVC only defines __AVX__ and __AVX2__
	// FMA3 was introduced alongside AVX2 on Intel Haswell
	// All AMD processors support FMA3 if AVX2 is also supported
	//
	// TURBO_FMA3 defines the level of FMA3 support:
	//  0 indicates no FMA3 support
	//  1 indicates FMA3 is supported
	#ifndef TURBO_FMA3
		#if defined(__FMA__) || TURBO_AVX2 >= 1
			#define TURBO_FMA3 1
		#else
			#define TURBO_FMA3 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// TURBO_TBM
	// TURBO_TBM may be used to determine if Trailing Bit Manipulation instructions are available for the target architecture
	#ifndef TURBO_TBM
		#if defined(__TBM__)
			#define TURBO_TBM 1
		#else
			#define TURBO_TBM 0
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_IMPORT
	// import declaration specification
	// specifies that the declared symbol is imported from another dynamic library.
	#ifndef TURBO_IMPORT
		#if defined(TURBO_COMPILER_MSVC)
			#define TURBO_IMPORT __declspec(dllimport)
		#else
			#define TURBO_IMPORT
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_EXPORT
	// export declaration specification
	// specifies that the declared symbol is exported from the current dynamic library.
	// this is not the same as the C++ export keyword. The C++ export keyword has been
	// removed from the language as of C++11.
	#ifndef TURBO_EXPORT
		#if defined(TURBO_COMPILER_MSVC)
			#define TURBO_EXPORT __declspec(dllexport)
		#else
			#define TURBO_EXPORT
		#endif
    #endif


///////////////////////////////////////////////////////////////////////////////
// TURBO_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If TURBO_DLL is defined, then the user is building EAThread as a DLL and EAThread's
// non-templated functions will be exported. EAThread template functions are not
// labelled as TURBO_API (and are thus not exported in a DLL build). This is
// because it's not possible (or at least unsafe) to implement inline templated
// functions in a DLL.
//
// Example usage of TURBO_API:
//    TURBO_API int someVariable = 10;         // Export someVariable in a DLL build.
//
//    struct TURBO_API SomeClass{              // Export SomeClass and its member functions in a DLL build.
//        TURBO_HIDDEN void PrivateMethod();    // Not exported.
//    };
//
//    TURBO_API void SomeFunction();           // Export SomeFunction in a DLL build.
//
// For GCC, see http://gcc.gnu.org/wiki/Visibility
//
#ifndef TURBO_API // If the build file hasn't already defined this to be dllexport...
#if defined(TURBO_DLL)
#if defined(_MSC_VER)
			#define TURBO_API      __declspec(dllimport)
			#define TURBO_HIDDEN
		#elif defined(__CYGWIN__)
			#define TURBO_API      __attribute__((dllimport))
			#define TURBO_HIDDEN
		#elif (defined(__GNUC__) && (__GNUC__ >= 4))
			#define TURBO_API      __attribute__ ((visibility("default")))
			#define TURBO_HIDDEN    __attribute__ ((visibility("hidden")))
		#else
			#define TURBO_API
			#define TURBO_HIDDEN
		#endif
#else
#define TURBO_API
#define TURBO_HIDDEN
#endif
#endif

#ifndef TURBO_INLINE_VISIBILITY
#define TURBO_INLINE_VISIBILITY  TURBO_HIDDEN TURBO_FORCE_INLINE
#endif  // TURBO_INLINE_VISIBILITY

    // ------------------------------------------------------------------------
	// TURBO_PRAGMA_ONCE_SUPPORTED
	//
	// This is a wrapper for the #pragma once preprocessor directive.
	// It allows for some compilers (in particular VC++) to implement signifcantly
	// faster include file preprocessing. #pragma once can be used to replace
	// header include guards or to augment them. However, #pragma once isn't
	// necessarily supported by all compilers and isn't guaranteed to be so in
	// the future, so using #pragma once to replace traditional include guards
	// is not strictly portable. Note that a direct #define for #pragma once is
	// impossible with VC++, due to limitations, but can be done with other
	// compilers/preprocessors via _Pragma("once").
	//
	// Example usage (which includes traditional header guards for portability):
	//    #ifndef SOMEPACKAGE_SOMEHEADER_H
	//    #define SOMEPACKAGE_SOMEHEADER_H
	//
	//    #if defined(TURBO_PRAGMA_ONCE_SUPPORTED)
	//        #pragma once
	//    #endif
	//
	//    <user code>
	//
	//    #endif
	//
	#if defined(_MSC_VER) || defined(__GNUC__) || defined(__EDG__) || defined(__APPLE__)
		#define TURBO_PRAGMA_ONCE_SUPPORTED 1
	#endif



	// ------------------------------------------------------------------------
	// TURBO_ONCE
	//
	// Example usage (which includes traditional header guards for portability):
	//    #ifndef SOMEPACKAGE_SOMEHEADER_H
	//    #define SOMEPACKAGE_SOMEHEADER_H
	//
	//    TURBO_ONCE()
	//
	//    <user code>
	//
	//    #endif
	//
	#if defined(TURBO_PRAGMA_ONCE_SUPPORTED)
		#if defined(_MSC_VER)
			#define TURBO_ONCE() __pragma(once)
		#else
			#define TURBO_ONCE() // _Pragma("once")   It turns out that _Pragma("once") isn't supported by many compilers.
		#endif
	#endif



	// ------------------------------------------------------------------------
	// TURBO_OVERRIDE
	//
	// C++11 override
	// See http://msdn.microsoft.com/en-us/library/jj678987.aspx for more information.
	// You can use TURBO_FINAL_OVERRIDE to combine usage of TURBO_OVERRIDE and TURBO_INHERITANCE_FINAL in a single statement.
	//
	// Example usage:
	//        struct B     { virtual void f(int); };
	//        struct D : B { void f(int) TURBO_OVERRIDE; };
	//
	#ifndef TURBO_OVERRIDE
		#if defined(TURBO_COMPILER_NO_OVERRIDE)
			#define TURBO_OVERRIDE
		#else
			#define TURBO_OVERRIDE override
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_INHERITANCE_FINAL
	//
	// Portably wraps the C++11 final specifier.
	// See http://msdn.microsoft.com/en-us/library/jj678985.aspx for more information.
	// You can use TURBO_FINAL_OVERRIDE to combine usage of TURBO_OVERRIDE and TURBO_INHERITANCE_FINAL in a single statement.
	// This is not called TURBO_FINAL because that term is used within EA to denote debug/release/final builds.
	//
	// Example usage:
	//     struct B { virtual void f() TURBO_INHERITANCE_FINAL; };
	//
	#ifndef TURBO_INHERITANCE_FINAL
		#if defined(TURBO_COMPILER_NO_INHERITANCE_FINAL)
			#define TURBO_INHERITANCE_FINAL
		#elif (defined(_MSC_VER) && (TURBO_COMPILER_VERSION < 1700))  // Pre-VS2012
			#define TURBO_INHERITANCE_FINAL sealed
		#else
			#define TURBO_INHERITANCE_FINAL final
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_FINAL_OVERRIDE
	//
	// Portably wraps the C++11 override final specifiers combined.
	//
	// Example usage:
	//     struct A            { virtual void f(); };
	//     struct B : public A { virtual void f() TURBO_FINAL_OVERRIDE; };
	//
	#ifndef TURBO_FINAL_OVERRIDE
		#define TURBO_FINAL_OVERRIDE TURBO_OVERRIDE TURBO_INHERITANCE_FINAL
	#endif


	// ------------------------------------------------------------------------
	// TURBO_SEALED
	//
	// This is deprecated, as the C++11 Standard has final (TURBO_INHERITANCE_FINAL) instead.
	// See http://msdn.microsoft.com/en-us/library/0w2w91tf.aspx for more information.
	// Example usage:
	//     struct B { virtual void f() TURBO_SEALED; };
	//
	#ifndef TURBO_SEALED
		#if defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION >= 1400) // VS2005 (VC8) and later
			#define TURBO_SEALED sealed
		#else
			#define TURBO_SEALED
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_ABSTRACT
	//
	// This is a Microsoft language extension.
	// See http://msdn.microsoft.com/en-us/library/b0z6b513.aspx for more information.
	// Example usage:
	//     struct X TURBO_ABSTRACT { virtual void f(){} };
	//
	#ifndef TURBO_ABSTRACT
		#if defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION >= 1400) // VS2005 (VC8) and later
			#define TURBO_ABSTRACT abstract
		#else
			#define TURBO_ABSTRACT
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_CONSTEXPR
	// TURBO_CONSTEXPR_OR_CONST
	//
	// Portable wrapper for C++11's 'constexpr' support.
	//
	// See http://www.cprogramming.com/c++11/c++11-compile-time-processing-with-constexpr.html for more information.
	// Example usage:
	//     TURBO_CONSTEXPR int GetValue() { return 37; }
	//     TURBO_CONSTEXPR_OR_CONST double gValue = std::sin(kTwoPi);
	//
	#if !defined(TURBO_CONSTEXPR)
		#if defined(TURBO_COMPILER_NO_CONSTEXPR)
			#define TURBO_CONSTEXPR
		#else
			#define TURBO_CONSTEXPR constexpr
		#endif
	#endif

	#if !defined(TURBO_CONSTEXPR_OR_CONST)
		#if defined(TURBO_COMPILER_NO_CONSTEXPR)
			#define TURBO_CONSTEXPR_OR_CONST const
		#else
			#define TURBO_CONSTEXPR_OR_CONST constexpr
		#endif
	#endif

	// ------------------------------------------------------------------------
	// TURBO_CONSTEXPR_IF
	//
	// Portable wrapper for C++17's 'constexpr if' support.
	//
	// https://en.cppreference.com/w/cpp/language/if
	//
	// Example usage:
	//
	// TURBO_CONSTEXPR_IF(tbstl::is_copy_constructible_v<T>)
	// 	{ ... }
	//
	#if !defined(TURBO_CONSTEXPR_IF)
		#if defined(TURBO_COMPILER_NO_CONSTEXPR_IF)
			#define TURBO_CONSTEXPR_IF(predicate) if ((predicate))
		#else
			#define TURBO_CONSTEXPR_IF(predicate) if constexpr ((predicate))
		#endif
	#endif



	// ------------------------------------------------------------------------
	// TURBO_EXTERN_TEMPLATE
	//
	// Portable wrapper for C++11's 'extern template' support.
	//
	// Example usage:
	//     TURBO_EXTERN_TEMPLATE(class basic_string<char>);
	//
	#if !defined(TURBO_EXTERN_TEMPLATE)
	#if defined(TURBO_COMPILER_NO_EXTERN_TEMPLATE)
		#define TURBO_EXTERN_TEMPLATE(declaration)
	#else
		#define TURBO_EXTERN_TEMPLATE(declaration) extern template declaration
	#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_NOEXCEPT
	// TURBO_NOEXCEPT_IF(predicate)
	// TURBO_NOEXCEPT_EXPR(expression)
	//
	// Portable wrapper for C++11 noexcept
	// http://en.cppreference.com/w/cpp/language/noexcept
	// http://en.cppreference.com/w/cpp/language/noexcept_spec
	//
	// Example usage:
	//     TURBO_NOEXCEPT
	//     TURBO_NOEXCEPT_IF(predicate)
	//     TURBO_NOEXCEPT_EXPR(expression)
	//
	//     This function never throws an exception.
	//     void DoNothing() TURBO_NOEXCEPT
	//         { }
	//
	//     This function throws an exception of T::T() throws an exception.
	//     template <class T>
	//     void DoNothing() TURBO_NOEXCEPT_IF(TURBO_NOEXCEPT_EXPR(T()))
	//         { T t; }
	//
	#if !defined(TURBO_NOEXCEPT)
		#if defined(TURBO_COMPILER_NO_NOEXCEPT)
			#define TURBO_NOEXCEPT
			#define TURBO_NOEXCEPT_IF(predicate)
			#define TURBO_NOEXCEPT_EXPR(expression) false
		#else
			#define TURBO_NOEXCEPT noexcept
			#define TURBO_NOEXCEPT_IF(predicate) noexcept((predicate))
			#define TURBO_NOEXCEPT_EXPR(expression) noexcept((expression))
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_NORETURN
	//
	// Wraps the C++11 noreturn attribute. See TURBO_COMPILER_NO_NORETURN
	// http://en.cppreference.com/w/cpp/language/attributes
	// http://msdn.microsoft.com/en-us/library/k6ktzx3s%28v=vs.80%29.aspx
	// http://blog.aaronballman.com/2011/09/understanding-attributes/
	//
	// Example usage:
	//     TURBO_NORETURN void SomeFunction()
	//         { throw "error"; }
	//
	#if !defined(TURBO_NORETURN)
		#if defined(TURBO_COMPILER_MSVC) && (TURBO_COMPILER_VERSION >= 1300) // VS2003 (VC7) and later
			#define TURBO_NORETURN __declspec(noreturn)
		#elif defined(TURBO_COMPILER_NO_NORETURN)
			#define TURBO_NORETURN
		#else
			#define TURBO_NORETURN [[noreturn]]
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_CARRIES_DEPENDENCY
	//
	// Wraps the C++11 carries_dependency attribute
	// http://en.cppreference.com/w/cpp/language/attributes
	// http://blog.aaronballman.com/2011/09/understanding-attributes/
	//
	// Example usage:
	//     TURBO_CARRIES_DEPENDENCY int* SomeFunction()
	//         { return &mX; }
	//
	//
	#if !defined(TURBO_CARRIES_DEPENDENCY)
		#if defined(TURBO_COMPILER_NO_CARRIES_DEPENDENCY)
			#define TURBO_CARRIES_DEPENDENCY
		#else
			#define TURBO_CARRIES_DEPENDENCY [[carries_dependency]]
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_FALLTHROUGH
	//
	// [[fallthrough] is a C++17 standard attribute that appears in switch
	// statements to indicate that the fallthrough from the previous case in the
	// switch statement is intentially and not a bug.
	//
	// http://en.cppreference.com/w/cpp/language/attributes
	//
	// Example usage:
	// 		void f(int n)
	// 		{
	// 			switch(n)
	// 			{
	// 				case 1:
	// 				DoCase1();
	// 				// Compiler may generate a warning for fallthrough behaviour
	//
	// 				case 2:
	// 				DoCase2();
	//
	// 				TURBO_FALLTHROUGH;
	// 				case 3:
	// 				DoCase3();
	// 			}
	// 		}
	//
	#if !defined(TURBO_FALLTHROUGH)
		#if defined(TURBO_COMPILER_NO_FALLTHROUGH)
			#define TURBO_FALLTHROUGH
		#else
			#define TURBO_FALLTHROUGH [[fallthrough]]
		#endif
	#endif



	// ------------------------------------------------------------------------
	// TURBO_NODISCARD
	//
	// [[nodiscard]] is a C++17 standard attribute that can be applied to a
	// function declaration, enum, or class declaration.  If a any of the list
	// previously are returned from a function (without the user explicitly
	// casting to void) the addition of the [[nodiscard]] attribute encourages
	// the compiler to generate a warning about the user discarding the return
	// value. This is a useful practice to encourage client code to check API
	// error codes.
	//
	// http://en.cppreference.com/w/cpp/language/attributes
	//
	// Example usage:
	//
	//     TURBO_NODISCARD int baz() { return 42; }
	//
	//     void foo()
	//     {
	//         baz(); // warning: ignoring return value of function declared with 'nodiscard' attribute
	//     }
	//
	#if !defined(TURBO_NODISCARD)
		#if defined(TURBO_COMPILER_NO_NODISCARD)
			#define TURBO_NODISCARD
		#else
			#define TURBO_NODISCARD [[nodiscard]]
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_MAYBE_UNUSED
	//
	// [[maybe_unused]] is a C++17 standard attribute that suppresses warnings
	// on unused entities that are declared as maybe_unused.
	//
	// http://en.cppreference.com/w/cpp/language/attributes
	//
	// Example usage:
	//    void foo(TURBO_MAYBE_UNUSED int i)
	//    {
	//        assert(i == 42);  // warning suppressed when asserts disabled.
	//    }
	//
	#if !defined(TURBO_MAYBE_UNUSED)
		#if defined(TURBO_COMPILER_NO_MAYBE_UNUSED)
			#define TURBO_MAYBE_UNUSED
		#else
			#define TURBO_MAYBE_UNUSED [[maybe_unused]]
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_NO_UBSAN
	//
	// The LLVM/Clang undefined behaviour sanitizer will not analyse a function tagged with the following attribute.
	//
	// https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#disabling-instrumentation-with-attribute-no-sanitize-undefined
	//
	// Example usage:
	//     TURBO_NO_UBSAN int SomeFunction() { ... }
	//
	#ifndef TURBO_NO_UBSAN
		#if defined(TURBO_COMPILER_CLANG)
			#define TURBO_NO_UBSAN __attribute__((no_sanitize("undefined")))
		#else
			#define TURBO_NO_UBSAN
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_NO_ASAN
	//
	// The LLVM/Clang address sanitizer will not analyse a function tagged with the following attribute.
	//
	// https://clang.llvm.org/docs/AddressSanitizer.html#disabling-instrumentation-with-attribute-no-sanitize-address
	//
	// Example usage:
	//     TURBO_NO_ASAN int SomeFunction() { ... }
	//
	#ifndef TURBO_NO_ASAN
		#if defined(TURBO_COMPILER_CLANG)
			#define TURBO_NO_ASAN __attribute__((no_sanitize("address")))
		#else
			#define TURBO_NO_ASAN
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_ASAN_ENABLED
	//
	// Defined as 0 or 1. It's value depends on the compile environment.
	// Specifies whether the code is being built with Clang's Address Sanitizer.
	//
	#if defined(__has_feature)
		#if __has_feature(address_sanitizer)
			#define TURBO_ASAN_ENABLED 1
		#else
			#define TURBO_ASAN_ENABLED 0
		#endif
	#else
		#define TURBO_ASAN_ENABLED 0
	#endif


	// ------------------------------------------------------------------------
	// TURBO_NON_COPYABLE
	//
	// This macro defines as a class as not being copy-constructable
	// or assignable. This is useful for preventing class instances
	// from being passed to functions by value, is useful for preventing
	// compiler warnings by some compilers about the inability to
	// auto-generate a copy constructor and assignment, and is useful
	// for simply declaring in the interface that copy semantics are
	// not supported by the class. Your class needs to have at least a
	// default constructor when using this macro.
	//
	// Beware that this class works by declaring a private: section of
	// the class in the case of compilers that don't support C++11 deleted
	// functions.
	//
	// Note: With some pre-C++11 compilers (e.g. Green Hills), you may need
	//       to manually define an instances of the hidden functions, even
	//       though they are not used.
	//
	// Example usage:
	//    class Widget {
	//       Widget();
	//       . . .
	//       TURBO_NON_COPYABLE(Widget)
	//    };
	//
	#if !defined(TURBO_NON_COPYABLE)
		#if defined(TURBO_COMPILER_NO_DELETED_FUNCTIONS)
			#define TURBO_NON_COPYABLE(EAClass_)               \
			  private:                                      \
				TURBO_DISABLE_VC_WARNING(4822);	/* local class member function does not have a body	*/		\
				EAClass_(const EAClass_&);                  \
				void operator=(const EAClass_&);			\
				TURBO_RESTORE_VC_WARNING();
		#else
			#define TURBO_NON_COPYABLE(EAClass_)               \
				TURBO_DISABLE_VC_WARNING(4822);	/* local class member function does not have a body	*/		\
				EAClass_(const EAClass_&) = delete;         \
				void operator=(const EAClass_&) = delete;	\
				TURBO_RESTORE_VC_WARNING();
		#endif
	#endif


	// ------------------------------------------------------------------------
	// TURBO_FUNCTION_DELETE
	//
	// Semi-portable way of specifying a deleted function which allows for
	// cleaner code in class declarations.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private: // For portability with pre-C++11 compilers, make the function private.
	//      void foo() TURBO_FUNCTION_DELETE;
	//  };
	//
	// Note: TURBO_FUNCTION_DELETE'd functions should be private to prevent the
	// functions from being called even when the compiler does not support
	// deleted functions. Some compilers (e.g. Green Hills) that don't support
	// C++11 deleted functions can require that you define the function,
	// which you can do in the associated source file for the class.
	//
	#if defined(TURBO_COMPILER_NO_DELETED_FUNCTIONS)
		#define TURBO_FUNCTION_DELETE
	#else
		#define TURBO_FUNCTION_DELETE = delete
	#endif

	// ------------------------------------------------------------------------
	// TURBO_DISABLE_DEFAULT_CTOR
	//
	// Disables the compiler generated default constructor. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      TURBO_DISABLE_DEFAULT_CTOR(Example);
	//  };
	//
	#define TURBO_DISABLE_DEFAULT_CTOR(ClassName) ClassName() TURBO_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// TURBO_DISABLE_COPY_CTOR
	//
	// Disables the compiler generated copy constructor. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      TURBO_DISABLE_COPY_CTOR(Example);
	//  };
	//
	#define TURBO_DISABLE_COPY_CTOR(ClassName) ClassName(const ClassName &) TURBO_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// TURBO_DISABLE_MOVE_CTOR
	//
	// Disables the compiler generated move constructor. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      TURBO_DISABLE_MOVE_CTOR(Example);
	//  };
	//
	#define TURBO_DISABLE_MOVE_CTOR(ClassName) ClassName(ClassName&&) TURBO_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// TURBO_DISABLE_ASSIGNMENT_OPERATOR
	//
	// Disables the compiler generated assignment operator. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      TURBO_DISABLE_ASSIGNMENT_OPERATOR(Example);
	//  };
	//
	#define TURBO_DISABLE_ASSIGNMENT_OPERATOR(ClassName) ClassName & operator=(const ClassName &) TURBO_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// TURBO_DISABLE_MOVE_OPERATOR
	//
	// Disables the compiler generated move operator. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      TURBO_DISABLE_MOVE_OPERATOR(Example);
	//  };
	//
	#define TURBO_DISABLE_MOVE_OPERATOR(ClassName) ClassName & operator=(ClassName&&) TURBO_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// EANonCopyable
	//
	// Declares a class as not supporting copy construction or assignment.
	// May be more reliable with some situations that TURBO_NON_COPYABLE alone,
	// though it may result in more code generation.
	//
	// Note that VC++ will generate warning C4625 and C4626 if you use EANonCopyable
	// and you are compiling with /W4 and /Wall. There is no resolution but
	// to redelare TURBO_NON_COPYABLE in your subclass or disable the warnings with
	// code like this:
	//     TURBO_DISABLE_VC_WARNING(4625 4626)
	//     ...
	//     TURBO_RESTORE_VC_WARNING()
	//
	// Example usage:
	//     struct Widget : EANonCopyable {
	//        . . .
	//     };
	//
	#ifdef __cplusplus
		struct EANonCopyable
		{
			#if defined(TURBO_COMPILER_NO_DEFAULTED_FUNCTIONS) ||  defined(__EDG__)
				// EDG doesn't appear to behave properly for the case of defaulted constructors;
				// it generates a mistaken warning about missing default constructors.
				EANonCopyable() {}  // Putting {} here has the downside that it allows a class to create itself,
				~EANonCopyable() {} // but avoids linker errors that can occur with some compilers (e.g. Green Hills).
			#else
				EANonCopyable() = default;
			   ~EANonCopyable() = default;
			#endif

			TURBO_NON_COPYABLE(EANonCopyable)
		};
	#endif


	// ------------------------------------------------------------------------
	// TURBO_OPTIMIZE_OFF / TURBO_OPTIMIZE_ON
	//
	// Implements portable inline optimization enabling/disabling.
	// Usage of these macros must be in order OFF then ON. This is
	// because the OFF macro pushes a set of settings and the ON
	// macro pops them. The nesting of OFF/ON sets (e.g. OFF, OFF, ON, ON)
	// is not guaranteed to work on all platforms.
	//
	// This is often used to allow debugging of some code that's
	// otherwise compiled with undebuggable optimizations. It's also
	// useful for working around compiler code generation problems
	// that occur in optimized builds.
	//
	// Some compilers (e.g. VC++) don't allow doing this within a function and
	// so the usage must be outside a function, as with the example below.
	// GCC on x86 appears to have some problem with argument passing when
	// using TURBO_OPTIMIZE_OFF in optimized builds.
	//
	// Example usage:
	//     // Disable optimizations for SomeFunction.
	//     TURBO_OPTIMIZE_OFF()
	//     void SomeFunction()
	//     {
	//         ...
	//     }
	//     TURBO_OPTIMIZE_ON()
	//
	#if !defined(TURBO_OPTIMIZE_OFF)
		#if   defined(TURBO_COMPILER_MSVC)
			#define TURBO_OPTIMIZE_OFF() __pragma(optimize("", off))
		#elif defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION > 4004) && (defined(__i386__) || defined(__x86_64__)) // GCC 4.4+ - Seems to work only on x86/Linux so far. However, GCC 4.4 itself appears broken and screws up parameter passing conventions.
			#define TURBO_OPTIMIZE_OFF()            \
				_Pragma("GCC push_options")      \
				_Pragma("GCC optimize 0")
        #elif defined(TURBO_COMPILER_CLANG) && (!defined(TURBO_PLATFORM_ANDROID) || (TURBO_COMPILER_VERSION >= 380))
            #define TURBO_OPTIMIZE_OFF() \
				TURBO_DISABLE_CLANG_WARNING(-Wunknown-pragmas) \
				_Pragma("clang optimize off") \
				TURBO_RESTORE_CLANG_WARNING()
		#else
			#define TURBO_OPTIMIZE_OFF()
		#endif
	#endif

	#if !defined(TURBO_OPTIMIZE_ON)
		#if   defined(TURBO_COMPILER_MSVC)
			#define TURBO_OPTIMIZE_ON() __pragma(optimize("", on))
		#elif defined(TURBO_COMPILER_GNUC) && (TURBO_COMPILER_VERSION > 4004) && (defined(__i386__) || defined(__x86_64__)) // GCC 4.4+ - Seems to work only on x86/Linux so far. However, GCC 4.4 itself appears broken and screws up parameter passing conventions.
			#define TURBO_OPTIMIZE_ON() _Pragma("GCC pop_options")
        #elif defined(TURBO_COMPILER_CLANG) && (!defined(TURBO_PLATFORM_ANDROID) || (TURBO_COMPILER_VERSION >= 380))
            #define TURBO_OPTIMIZE_ON() \
				TURBO_DISABLE_CLANG_WARNING(-Wunknown-pragmas) \
				_Pragma("clang optimize on") \
				TURBO_RESTORE_CLANG_WARNING()
		#else
			#define TURBO_OPTIMIZE_ON()
		#endif
	#endif


    #ifndef TURBO_ALLOW_UNUSED
        #if TURBO_HAVE_ATTRIBUTE(unused)
            #define TURBO_ALLOW_UNUSED __attribute__((unused))
        #else
            #define TURBO_ALLOW_UNUSED
        #endif
    #endif  // TURBO_ALLOW_UNUSED


    // TURBO_MUST_USE_RESULT
    //
    // Tells the compiler to warn about unused results.
    //
    // For code or headers that are assured to only build with C++17 and up, prefer
    // just using the standard `[[nodiscard]]` directly over this macro.
    //
    // When annotating a function, it must appear as the first part of the
    // declaration or definition. The compiler will warn if the return value from
    // such a function is unused:
    //
    //   TURBO_MUST_USE_RESULT Sprocket* AllocateSprocket();
    //   AllocateSprocket();  // Triggers a warning.
    //
    // When annotating a class, it is equivalent to annotating every function which
    // returns an instance.
    //
    //   class TURBO_MUST_USE_RESULT Sprocket {};
    //   Sprocket();  // Triggers a warning.
    //
    //   Sprocket MakeSprocket();
    //   MakeSprocket();  // Triggers a warning.
    //
    // Note that references and pointers are not instances:
    //
    //   Sprocket* SprocketPointer();
    //   SprocketPointer();  // Does *not* trigger a warning.
    //
    // TURBO_MUST_USE_RESULT allows using cast-to-void to suppress the unused result
    // warning. For that, warn_unused_result is used only for clang but not for gcc.
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425
    //
    // Note: past advice was to place the macro after the argument list.
    //
    // TODO(b/176172494): Use TURBO_HAVE_CPP_ATTRIBUTE(nodiscard) when all code is
    // compliant with the stricter [[nodiscard]].
    #if defined(__clang__) && TURBO_HAVE_ATTRIBUTE(warn_unused_result)
        #define TURBO_MUST_USE_RESULT __attribute__((warn_unused_result))
    #else
        #define TURBO_MUST_USE_RESULT
    #endif

#endif  // TURBO_PLATFORM_CONFIG_COMPILER_TRAITS_H_










