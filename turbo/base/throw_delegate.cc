
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "turbo/base/throw_delegate.h"
#include <cstdlib>
#include <functional>
#include <new>
#include <stdexcept>
#include "turbo/base/profile.h"
#include "turbo/log/logging.h"

namespace turbo::base {

    namespace {
        template<typename T>
        [[noreturn]] void do_throw(const T &error) {
#ifdef TURBO_HAVE_EXCEPTIONS
            throw error;
#else
            TURBO_DLOG(FATAL) << error.what();
            std::abort();
#endif
        }
    }  // namespace

    void throw_std_logic_error(const std::string &what_arg) {
        do_throw(std::logic_error(what_arg));
    }

    void throw_std_logic_error(const char *what_arg) {
        do_throw(std::logic_error(what_arg));
    }

    void throw_std_invalid_argument(const std::string &what_arg) {
        do_throw(std::invalid_argument(what_arg));
    }

    void throw_std_invalid_argument(const char *what_arg) {
        do_throw(std::invalid_argument(what_arg));
    }

    void throw_std_domain_error(const std::string &what_arg) {
        do_throw(std::domain_error(what_arg));
    }

    void throw_std_domain_error(const char *what_arg) {
        do_throw(std::domain_error(what_arg));
    }

    void throw_std_length_error(const std::string &what_arg) {
        do_throw(std::length_error(what_arg));
    }

    void throw_std_length_error(const char *what_arg) {
        do_throw(std::length_error(what_arg));
    }

    void throw_std_out_of_range(const std::string &what_arg) {
        do_throw(std::out_of_range(what_arg));
    }

    void throw_std_out_of_range(const char *what_arg) {
        do_throw(std::out_of_range(what_arg));
    }

    void throw_std_runtime_error(const std::string &what_arg) {
        do_throw(std::runtime_error(what_arg));
    }

    void throw_std_runtime_error(const char *what_arg) {
        do_throw(std::runtime_error(what_arg));
    }

    void throw_std_range_error(const std::string &what_arg) {
        do_throw(std::range_error(what_arg));
    }

    void throw_std_range_error(const char *what_arg) {
        do_throw(std::range_error(what_arg));
    }

    void throw_std_overflow_error(const std::string &what_arg) {
        do_throw(std::overflow_error(what_arg));
    }

    void throw_std_overflow_error(const char *what_arg) {
        do_throw(std::overflow_error(what_arg));
    }

    void throw_std_underflow_error(const std::string &what_arg) {
        do_throw(std::underflow_error(what_arg));
    }

    void throw_std_underflow_error(const char *what_arg) {
        do_throw(std::underflow_error(what_arg));
    }

    void throw_std_bad_function_call() { do_throw(std::bad_function_call()); }

    void throw_std_bad_alloc() { do_throw(std::bad_alloc()); }


}  // namespace turbo::base
