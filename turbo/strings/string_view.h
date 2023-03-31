//
// Created by 李寅斌 on 2023/3/4.
//

#ifndef TURBO_STRINGS_STRING_VIEW_H_
#define TURBO_STRINGS_STRING_VIEW_H_
#include "turbo/platform/port.h"



// After standard library includes.
// Standard library support for std::string_view.
#if defined(__cpp_lib_string_view)
#define TURBO_HAS_STD_STRING_VIEW
#elif defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION >= 4000) && (__cplusplus >= 201402)
#define TURBO_HAS_STD_STRING_VIEW
#elif defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 7) && (__cplusplus >= 201703)
#define TURBO_HAS_STD_STRING_VIEW
#elif defined(_MSC_VER) && (_MSC_VER >= 1910 && _MSVC_LANG >= 201703)
#define TURBO_HAS_STD_STRING_VIEW
#endif

// Standard library support for std::experimental::string_view.
#if defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION >= 3700 && _LIBCPP_VERSION < 7000) && (__cplusplus >= 201402)
#define TURBO_HAS_STD_EXPERIMENTAL_STRING_VIEW
#elif defined(__GNUC__) && (((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)) || (__GNUC__ > 4)) && (__cplusplus >= 201402)
#define TURBO_HAS_STD_EXPERIMENTAL_STRING_VIEW
#elif defined(__GLIBCXX__) && defined(_GLIBCXX_USE_DUAL_ABI) && (__cplusplus >= 201402)
// macro _GLIBCXX_USE_DUAL_ABI is always defined in libstdc++ from gcc-5 and newer
#define TURBO_HAS_STD_EXPERIMENTAL_STRING_VIEW
#endif


#if defined(TURBO_HAS_STD_STRING_VIEW)
#include <string_view>
#elif defined(TURBO_HAS_STD_EXPERIMENTAL_STRING_VIEW)
#include <experimental/string_view>
#endif
namespace turbo {

#if defined(TURBO_HAS_STD_STRING_VIEW)
#define TURBO_WITH_STRING_VIEW
using std::basic_string_view;
using std::string_view;
using std::wstring_view;
#elif defined(TURBO_HAS_STD_EXPERIMENTAL_STRING_VIEW)
#define TURBO_WITH_STRING_VIEW
using std::experimental::basic_string_view;
using std::experimental::string_view;
using std::experimental::wstring_view;
#endif

}  // namespace turbo

#endif // TURBO_STRINGS_STRING_VIEW_H_
