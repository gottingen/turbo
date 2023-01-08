
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_BASE_RESULT_STATUS_H_
#define FLARE_BASE_RESULT_STATUS_H_

#include "flare/strings/str_format.h"

namespace flare {
    class result_status {
    public:
        result_status() = default;

        ~result_status() {
            reset();
        }

        result_status(const result_status &) = default;

        result_status &operator=(const result_status &) = default;

        result_status(result_status &&) = default;

        result_status &operator=(result_status &&) = default;

        result_status(int err, const std::string_view &msg)
                : _error(err), _error_msg(msg) {}

        template<typename ...Args>
        result_status(int err, const std::string_view &fmt, Args &&... args) :_error(err), _error_msg() {
            _error_msg = string_format(fmt, std::forward<Args>(args)...);
        }

        void reset() {
            _error = 0;
            _error_msg.clear();
        }

        void set_error(int err, const std::string_view &msg) {
            _error = err;
            _error_msg.assign(msg.data(), msg.size());
        }

        template<typename ...Args>
        void set_error(int err, const std::string_view &fmt, Args &&... args) {
            _error = err;
            _error_msg = string_format(fmt, std::forward<Args>(args)...);
        }

        explicit operator bool() const {
            return _error == 0;
        }

        bool is_ok() const {
            return _error == 0;
        }

        int error_code() const {
            return _error;
        }

        void swap(result_status &rhs) {
            std::swap(_error, rhs._error);
            std::swap(_error_msg, rhs._error_msg);
        }

        const char* error_cstr() const {
            return _error == 0 ? "OK" : _error_msg.c_str();
        }

        const std::string &error_str() const {
            return error_data();
        }

        const std::string &error_data() const {
            static std::string ok_str = "OK";
            if(_error == 0) {
                return ok_str;
            }
            return _error_msg;
        }

        static result_status success() {
            static result_status ok;
            return ok;
        }

        static result_status from_flare_error(int err);

        static result_status from_flare_error(int err, const std::string_view &ext);

        template<typename ...Args>
        static result_status from_flare_error(int err, const std::string_view &fmt, Args &&...args) {
            auto msg = string_format(fmt, std::forward<Args>(args)...);
            return from_flare_error(err, msg);
        }

        static result_status from_error_code(const std::error_code &ec);

        static result_status from_last_error();

    private:
        int _error{0};
        std::string _error_msg;
    };

    inline std::ostream &operator<<(std::ostream &os, const result_status &st) {
        // NOTE: don't use st.error_text() which is inaccurate if message has '\0'
        return os << st.error_data();
    }

}  // namespace flare

#endif  // FLARE_BASE_RESULT_STATUS_H_
