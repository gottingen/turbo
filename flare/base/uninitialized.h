
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_UTILITY_UNINITIALIZED_H_
#define FLARE_UTILITY_UNINITIALIZED_H_

#include <string>
#include <type_traits>
#include <utility>
#include "flare/base/profile.h"
#include "flare/base/type_traits.h"  //  for void_t

namespace flare::base {


    // Is a subclass of true_type or false_type, depending on whether or not
    // T has a __resize_default_init member.
    template<typename string_type, typename = void>
    struct resize_uninitialized_traits {
        using HasMember = std::false_type;

        static void resize(string_type *s, size_t new_size) { s->resize(new_size); }
    };

// __resize_default_init is provided by libc++ >= 8.0
    template<typename string_type>
    struct resize_uninitialized_traits<
            string_type, std::void_t < decltype(std::declval<string_type &>()
                    .__resize_default_init(237))> > {
    using HasMember = std::true_type;

    static void resize(string_type *s, size_t new_size) {
        s->__resize_default_init(new_size);
    }
};

// Returns true if the std::string implementation supports a resize where
// the new characters added to the std::string are left untouched.
//
template<typename string_type>
FLARE_FORCE_INLINE constexpr bool string_supports_uninitialized_resize(string_type *) {
    return resize_uninitialized_traits<string_type>::HasMember::value;
}

// Like str->resize(new_size), except any new characters added to "*str" as a
// result of resizing may be left uninitialized, rather than being filled with
// '0' bytes. Typically used when code is then going to overwrite the backing
// store of the std::string with known data.
template<typename string_type, typename = void>
FLARE_FORCE_INLINE void string_resize_uninitialized(string_type *s, size_t new_size) {
    resize_uninitialized_traits<string_type>::resize(s, new_size);
}


}  // namespace flare::base
#endif  // FLARE_UTILITY_UNINITIALIZED_H_
