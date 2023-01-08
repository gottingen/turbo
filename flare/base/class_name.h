
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_CLASS_NAME_H_
#define FLARE_BASE_CLASS_NAME_H_

#include <typeinfo>
#include <string>                                // std::string

namespace flare::base {

    std::string demangle(const char *name);

    namespace detail {
        template<typename T>
        struct ClassNameHelper {
            static std::string name;
        };
        template<typename T> std::string ClassNameHelper<T>::name = demangle(typeid(T).name());
    }

    // Get name of class |T|, in std::string.
    template<typename T>
    const std::string &class_name_str() {
        // We don't use static-variable-inside-function because before C++11
        // local static variable is not guaranteed to be thread-safe.
        return detail::ClassNameHelper<T>::name;
    }

    // Get name of class |T|, in const char*.
    // Address of returned name never changes.
    template<typename T>
    const char *class_name() {
        return class_name_str<T>().c_str();
    }

    // Get typename of |obj|, in std::string
    template<typename T>
    std::string class_name_str(T const &obj) {
        return demangle(typeid(obj).name());
    }

}  // namespace flare::base

#endif  // FLARE_BASE_CLASS_NAME_H_
