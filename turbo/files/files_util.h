
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_FILES_FILES_UTIL_H_
#define TURBO_FILES_FILES_UTIL_H_

#include "turbo/base/type_traits.h"
#include "turbo/files/filesystem.h"
#include "turbo/files/internal/list_sirectory.h"

namespace turbo::files_internal {

    template<typename C, bool has_value_type>
    struct collector_is_convertible_to_impl : std::false_type {
    };

    template<typename C>
    struct collector_is_convertible_to_impl<C, true>
            : std::is_constructible<typename C::value_type, std::string> {
    };

    template<typename C>
    struct collector_is_convertible_to
            : collector_is_convertible_to_impl<C,
                    turbo::has_value_type<C>::value &&
                    !std::is_same<typename C::value_type, std::string_view>::value
            > {
    };

    template<typename Predicate>
    class files_collector {
    public:
        files_collector(std::string_view path, Predicate p, bool re) : _files(), _predicate(std::move(p)) {
            _files = list_directory_internal(path, re, _error);
        }

        template<typename Container, typename = typename std::enable_if<
                turbo::has_value_type<Container>::value &&
                std::is_same<typename Container::value_type, std::string_view>::value == false &&
                (std::is_constructible<typename Container::value_type, std::string>::value ||
                 std::is_constructible<typename Container::value_type, std::string_view>::value ||
                 std::is_constructible<typename Container::value_type, turbo::file_path>::value)>::type>
        operator Container() {
            Container c;
            using ValueType = typename Container::value_type;
            if (_error) {
                return c;
            }
            auto it = std::inserter(c, c.end());
            for (const auto &sp : _files) {
                std::string p = sp.generic_string();
                if (_predicate(p, _error) && !_error) {
                    *it++ = ValueType(std::move(p));
                }
                if (_error) {
                    return c;
                }
            }
            return c;
        }

        std::error_code error() const {
            return _error;
        }

    private:
        std::error_code _error;
        std::vector<file_path> _files;
        Predicate _predicate;
    };
}  // namespace turbo::files_internal

namespace turbo {

    struct file_and_directory {
        bool operator()(const file_path &path, std::error_code &ec) {
            return true;
        }
    };

    struct only_directory {
        bool operator()(const file_path &path, std::error_code &ec) {
            return turbo::is_directory(path, ec);
        }
    };

    struct only_file {
        bool operator()(const file_path &path, std::error_code &ec) {
            return turbo::is_regular_file(path, ec);
        }
    };


    template<typename Predicate>
    files_internal::files_collector<Predicate>
    list_directory(std::string_view path, std::error_code &ec, Predicate p) noexcept {
        files_internal::files_collector<Predicate> fl(path, std::move(p), false);
        ec = fl.error();
        return fl;
    }

    template<typename Predicate>
    files_internal::files_collector<Predicate>
    list_directory_recursive(std::string_view path, std::error_code &ec, Predicate p) noexcept {
        files_internal::files_collector<Predicate> fl(path, std::move(p), true);
        ec = fl.error();
        return fl;
    }

    template<typename Container, typename ValueType =
    typename std::enable_if<turbo::has_value_type<Container>::value &&
                            (std::is_same<typename Container::value_type, std::string>::value ||
                             std::is_same<typename Container::value_type, std::string_view>::value ||
                             std::is_same<typename Container::value_type, file_path>::value
                            ), typename Container::value_type>::type>
    file_path join_path(const Container &c) {
        file_path path;
        for (auto it : c) {
            path /= it;
        }
        return path;
    }

    template<typename Container, typename ValueType =
    typename std::enable_if<turbo::has_value_type<Container>::value &&
                            (std::is_same<typename Container::value_type, std::string>::value ||
                             std::is_same<typename Container::value_type, std::string_view>::value ||
                             std::is_same<typename Container::value_type, file_path>::value
                            ), typename Container::value_type>::type>
    file_path join_path(const file_path &p, const Container &c) {
        file_path path(p);
        for (auto it : c) {
            path /= it;
        }
        return path;
    }
}  // namespace turbo

#endif  // TURBO_FILES_FILES_UTIL_H_
