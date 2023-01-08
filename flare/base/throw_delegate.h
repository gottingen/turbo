
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_THROW_DELEGATE_H_
#define FLARE_BASE_THROW_DELEGATE_H_

#include <string>
#include "flare/base/profile.h"

namespace flare::base {


    // Helper functions that allow throwing exceptions consistently from anywhere.
    // The main use case is for header-based libraries (eg templates), as they will
    // be built by many different targets with their own compiler options.
    // In particular, this will allow a safe way to throw exceptions even if the
    // caller is compiled with -fno-exceptions.  This is intended for implementing
    // things like map<>::at(), which the standard documents as throwing an
    // exception on error.
    //
    // Using other techniques like #if tricks could lead to ODR violations.
    //
    // You shouldn't use it unless you're writing code that you know will be built
    // both with and without exceptions and you need to conform to an interface
    // that uses exceptions.

    [[noreturn]] void throw_std_logic_error(const std::string &what_arg);

    [[noreturn]] void throw_std_logic_error(const char *what_arg);

    [[noreturn]] void throw_std_invalid_argument(const std::string &what_arg);

    [[noreturn]] void throw_std_invalid_argument(const char *what_arg);

    [[noreturn]] void throw_std_domain_error(const std::string &what_arg);

    [[noreturn]] void throw_std_domain_error(const char *what_arg);

    [[noreturn]] void throw_std_length_error(const std::string &what_arg);

    [[noreturn]] void throw_std_length_error(const char *what_arg);

    [[noreturn]] void throw_std_out_of_range(const std::string &what_arg);

    [[noreturn]] void throw_std_out_of_range(const char *what_arg);

    [[noreturn]] void throw_std_runtime_error(const std::string &what_arg);

    [[noreturn]] void throw_std_runtime_error(const char *what_arg);

    [[noreturn]] void throw_std_range_error(const std::string &what_arg);

    [[noreturn]] void throw_std_range_error(const char *what_arg);

    [[noreturn]] void throw_std_overflow_error(const std::string &what_arg);

    [[noreturn]] void throw_std_overflow_error(const char *what_arg);

    [[noreturn]] void throw_std_underflow_error(const std::string &what_arg);

    [[noreturn]] void throw_std_underflow_error(const char *what_arg);

    [[noreturn]] void throw_std_bad_function_call();

    [[noreturn]] void throw_std_bad_alloc();

    // ThrowStdBadArrayNewLength() cannot be consistently supported because
    // std::bad_array_new_length is missing in libstdc++ until 4.9.0.
    // https://gcc.gnu.org/onlinedocs/gcc-4.8.3/libstdc++/api/a01379_source.html
    // https://gcc.gnu.org/onlinedocs/gcc-4.9.0/libstdc++/api/a01327_source.html
    // libcxx (as of 3.2) and msvc (as of 2015) both have it.
    // [[noreturn]] void ThrowStdBadArrayNewLength();


}  // namespace flare::base

#endif  // FLARE_BASE_THROW_DELEGATE_H_
