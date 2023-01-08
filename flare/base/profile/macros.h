
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_BASE_PROFILE_MACROS_H_
#define FLARE_BASE_PROFILE_MACROS_H_

// Concatenate numbers in c/c++ macros.
#ifndef FLARE_CONCAT
# define FLARE_CONCAT(a, b) FLARE_CONCAT_HELPER(a, b)
# define FLARE_CONCAT_HELPER(a, b) a##b
#endif

#define FLARE_DELETE_FUNCTION(decl) decl = delete

#define FLARE_DISALLOW_COPY_AND_ASSIGN(TypeName)                      \
    FLARE_DELETE_FUNCTION(TypeName(const TypeName&));            \
    FLARE_DELETE_FUNCTION(void operator=(const TypeName&))



// DEFINE_SMALL_ARRAY(MyType, my_array, size, 64);
//   my_array is typed `MyType*' and as long as `size'. If `size' is not
//   greater than 64, the array is allocated on stack.
//
// NOTE: NEVER use ARRAY_SIZE(my_array) which is always 1.

#if defined(__cplusplus)
namespace flare::base {
    namespace internal {
        template<typename T>
        struct ArrayDeleter {
            ArrayDeleter() : arr(0) {}

            ~ArrayDeleter() { delete[] arr; }

            T *arr;
        };
    }
}

// Many versions of clang does not support variable-length array with non-pod
// types, have to implement the macro differently.
#if !defined(__clang__)
# define DEFINE_SMALL_ARRAY(Tp, name, size, maxsize)                    \
    Tp* name = 0;                                                       \
    const unsigned name##_size = (size);                                \
    const unsigned name##_stack_array_size = (name##_size <= (maxsize) ? name##_size : 0); \
    Tp name##_stack_array[name##_stack_array_size];                     \
    ::flare::base::internal::ArrayDeleter<Tp> name##_array_deleter;            \
    if (name##_stack_array_size) {                                      \
        name = name##_stack_array;                                      \
    } else {                                                            \
        name = new (::std::nothrow) Tp[name##_size];                    \
        name##_array_deleter.arr = name;                                \
    }
#else
// This implementation works for GCC as well, however it needs extra 16 bytes
// for ArrayCtorDtor.
namespace flare::base {
    namespace internal {
        template<typename T>
        struct ArrayCtorDtor {
            ArrayCtorDtor(void *arr, unsigned size) : _arr((T *) arr), _size(size) {
                for (unsigned i = 0; i < size; ++i) { new(_arr + i) T; }
            }

            ~ArrayCtorDtor() {
                for (unsigned i = 0; i < _size; ++i) { _arr[i].~T(); }
            }

        private:
            T *_arr;
            unsigned _size;
        };
    }
}
# define DEFINE_SMALL_ARRAY(Tp, name, size, maxsize)                    \
    Tp* name = 0;                                                       \
    const unsigned name##_size = (size);                                \
    const unsigned name##_stack_array_size = (name##_size <= (maxsize) ? name##_size : 0); \
    char name##_stack_array[sizeof(Tp) * name##_stack_array_size];      \
    ::flare::base::internal::ArrayDeleter<char> name##_array_deleter;          \
    if (name##_stack_array_size) {                                      \
        name = (Tp*)name##_stack_array;                                 \
    } else {                                                            \
        name = (Tp*)new (::std::nothrow) char[sizeof(Tp) * name##_size];\
        name##_array_deleter.arr = (char*)name;                         \
    }                                                                   \
    const ::flare::base::internal::ArrayCtorDtor<Tp> name##_array_ctor_dtor(name, name##_size);
#endif // !defined(__clang__)
#endif // defined(__cplusplus)


// Convert symbol to string
#ifndef FLARE_SYMBOLSTR
# define FLARE_SYMBOLSTR(a) FLARE_SYMBOLSTR_HELPER(a)
# define FLARE_SYMBOLSTR_HELPER(a) #a
#endif

// ptr:     the pointer to the member.
// type:    the type of the container struct this is embedded in.
// member:  the name of the member within the struct.
#ifndef FLARE_CONTAINER_OF
# define FLARE_CONTAINER_OF(ptr, type, member) ({                             \
            const decltype( ((type *)0)->member ) *__mptr = (ptr);  \
            (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

namespace flare::base {
    template<typename T>
    inline void ignore_result(const T &) {
    }
} // namespace flare::base



// FLARE_HAVE_INTRINSIC_INT128
//
// Checks whether the __int128 compiler extension for a 128-bit integral type is
// supported.
//
// Note: __SIZEOF_INT128__ is defined by Clang and GCC when __int128 is
// supported, but we avoid using it in certain cases:
// * On Clang:
//   * Building using Clang for Windows, where the Clang runtime library has
//     128-bit support only on LP64 architectures, but Windows is LLP64.
// * On Nvidia's nvcc:
//   * nvcc also defines __GNUC__ and __SIZEOF_INT128__, but not all versions
//     actually support __int128.
#ifdef FLARE_HAVE_INTRINSIC_INT128
#error FLARE_HAVE_INTRINSIC_INT128 cannot be directly set
#elif defined(__SIZEOF_INT128__)
#if (defined(__clang__) && !defined(_WIN32)) || \
            (defined(__CUDACC__) && __CUDACC_VER_MAJOR__ >= 9) || \
            (defined(__GNUC__) && !defined(__clang__) && !defined(__CUDACC__))
#define FLARE_HAVE_INTRINSIC_INT128 1
#elif defined(__CUDACC__)
// __CUDACC_VER__ is a full version number before CUDA 9, and is defined to a
// string explaining that it has been removed starting with CUDA 9. We use
// nested #ifs because there is no short-circuiting in the preprocessor.
// NOTE: `__CUDACC__` could be undefined while `__CUDACC_VER__` is defined.
#if __CUDACC_VER__ >= 70000
#define FLARE_HAVE_INTRINSIC_INT128 1
#endif  // __CUDACC_VER__ >= 70000
#endif  // defined(__CUDACC__)
#endif  // FLARE_HAVE_INTRINSIC_INT128


// FLARE_FALLTHROUGH_INTENDED
//
// Annotates implicit fall-through between switch labels, allowing a case to
// indicate intentional fallthrough and turn off warnings about any lack of a
// `break` statement. The FLARE_FALLTHROUGH_INTENDED macro should be followed by
// a semicolon and can be used in most places where `break` can, provided that
// no statements exist between it and the next switch label.
//
// Example:
//
//  switch (x) {
//    case 40:
//    case 41:
//      if (truth_is_out_there) {
//        ++x;
//        FLARE_FALLTHROUGH_INTENDED;  // Use instead of/along with annotations
//                                    // in comments
//      } else {
//        return x;
//      }
//    case 42:
//      ...
//
// Notes: when compiled with clang in C++11 mode, the FLARE_FALLTHROUGH_INTENDED
// macro is expanded to the [[clang::fallthrough]] attribute, which is analysed
// when  performing switch labels fall-through diagnostic
// (`-Wimplicit-fallthrough`). See clang documentation on language extensions
// for details:
// http://clang.llvm.org/docs/AttributeReference.html#fallthrough-clang-fallthrough
//
// When used with unsupported compilers, the FLARE_FALLTHROUGH_INTENDED macro
// has no effect on diagnostics. In any case this macro has no effect on runtime
// behavior and performance of code.
#ifdef FLARE_FALLTHROUGH_INTENDED
#error "FLARE_FALLTHROUGH_INTENDED should not be defined."
#endif

// TODO(zhangxy): Use c++17 standard [[fallthrough]] macro, when supported.
#if defined(__clang__) && defined(__has_warning)
#if __has_feature(cxx_attributes) && __has_warning("-Wimplicit-fallthrough")
#define FLARE_FALLTHROUGH_INTENDED [[clang::fallthrough]]
#endif
#elif defined(__GNUC__) && __GNUC__ >= 7
#define FLARE_FALLTHROUGH_INTENDED [[gnu::fallthrough]]
#endif

#ifndef FLARE_FALLTHROUGH_INTENDED
#define FLARE_FALLTHROUGH_INTENDED \
  do {                            \
  } while (0)
#endif



// FLARE_BLOCK_TAIL_CALL_OPTIMIZATION
//
// Instructs the compiler to avoid optimizing tail-call recursion. Use of this
// macro is useful when you wish to preserve the existing function order within
// a stack trace for logging, debugging, or profiling purposes.
//
// Example:
//
//   int f() {
//     int result = g();
//     FLARE_BLOCK_TAIL_CALL_OPTIMIZATION();
//     return result;
//   }
#if defined(__pnacl__)
#define FLARE_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#elif defined(__clang__)
// Clang will not tail call given inline volatile assembly.
#define FLARE_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(__GNUC__)
// GCC will not tail call given inline volatile assembly.
#define FLARE_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(_MSC_VER)
#include <intrin.h>
// The __nop() intrinsic blocks the optimisation.
#define FLARE_BLOCK_TAIL_CALL_OPTIMIZATION() __nop()
#else
#define FLARE_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#endif

#ifdef _MSC_VER
#define FLARE_THREAD_LOCAL __declspec(thread)
#else
#define FLARE_THREAD_LOCAL __thread
#endif  // _MSC_VER


// ------------------------------------------------------------------
// Checks whether the compiler both supports and enables exceptions.
// ------------------------------------------------------------------
#ifdef FLARE_HAVE_EXCEPTIONS
#error FLARE_HAVE_EXCEPTIONS cannot be directly set.
#elif defined(__clang__)
#if defined(__EXCEPTIONS) && __has_feature(cxx_exceptions)
#define FLARE_HAVE_EXCEPTIONS 1
#endif  // defined(__EXCEPTIONS) && __has_feature(cxx_exceptions)
#elif !(defined(__GNUC__) && (__GNUC__ < 5) && !defined(__EXCEPTIONS)) && \
    !(defined(__GNUC__) && (__GNUC__ >= 5) && !defined(__cpp_exceptions)) && \
    !(defined(_MSC_VER) && !defined(_CPPUNWIND))
#define FLARE_HAVE_EXCEPTIONS 1
#endif

#ifdef FLARE_HAVE_EXCEPTIONS
#define FLARE_INTERNAL_TRY try
#define FLARE_INTERNAL_CATCH_ANY catch (...)
#define FLARE_INTERNAL_RETHROW do { throw; } while (false)
#else  // FLARE_HAVE_EXCEPTIONS
#define FLARE_INTERNAL_TRY if (true)
#define FLARE_INTERNAL_CATCH_ANY else if (false)
#define FLARE_INTERNAL_RETHROW do {} while (false)
#endif  // FLARE_HAVE_EXCEPTIONS


// ----------------------------------------------------------------------
// Figure out SSE support
// ----------------------------------------------------------------------
#ifndef FLARE_HAVE_SSE2
#if defined(__SSE2__) || \
        (defined(_MSC_VER) && \
         (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 2)))
#define FLARE_HAVE_SSE2 1
#else
#define FLARE_HAVE_SSE2 0
#endif
#endif

#ifndef FLARE_HAVE_SSSE3
#if defined(__SSSE3__) || defined(__AVX2__)
#define FLARE_HAVE_SSSE3 1
#else
#define FLARE_HAVE_SSSE3 0
#endif
#endif

#if FLARE_HAVE_SSSE3 && !FLARE_HAVE_SSE2
#error "Bad configuration!"
#endif

#if FLARE_HAVE_SSE2

#include <emmintrin.h>

#endif

#if FLARE_HAVE_SSSE3

#include <tmmintrin.h>

#endif


// FLARE_ASSERT()
//
// In C++11, `assert` can't be used portably within constexpr functions.
// FLARE_ASSERT functions as a runtime assert but works in C++11 constexpr
// functions.  Example:
//
// constexpr double Divide(double a, double b) {
//   return FLARE_ASSERT(b != 0), a / b;
// }
//
// This macro is inspired by
// https://akrzemi1.wordpress.com/2017/05/18/asserts-in-constexpr-functions/
#if defined(NDEBUG)
#define FLARE_ASSERT(expr) (false ? (void)(expr) : (void)0)
#else
#define FLARE_ASSERT(expr)              \
      (FLARE_LIKELY((expr)) ? (void)0 \
                                 : [] { assert(false && #expr); }())  // NOLINT
#endif


#endif  // FLARE_BASE_PROFILE_MACROS_H_
