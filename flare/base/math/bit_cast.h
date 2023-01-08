
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_BIT_CAST_H_
#define FLARE_BASE_MATH_BIT_CAST_H_


#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#include "flare/base/profile.h"

namespace flare::base {

    namespace math_internal {

        template<class Dest, class Source>
        struct is_bitcastable
                : std::integral_constant<
                        bool,
                        sizeof(Dest) == sizeof(Source) &&
                        std::is_trivially_copyable<Source>::value &&
                        std::is_trivially_copyable<Dest>::value &&
                        std::is_default_constructible<Dest>::value> {
        };

    }  // namespace math_internal


    // bit_cast()
    //
    // Performs a bitwise cast on a type without changing the underlying bit
    // representation of that type's value. The two types must be of the same size
    // and both types must be trivially copyable. As with most casts, use with
    // caution. A `bit_cast()` might be needed when you need to temporarily treat a
    // type as some other type, such as in the following cases:
    //
    //    * Serialization (casting temporarily to `char *` for those purposes is
    //      always allowed by the C++ standard)
    //    * Managing the individual bits of a type within mathematical operations
    //      that are not normally accessible through that type
    //    * Casting non-pointer types to pointer types (casting the other way is
    //      allowed by `reinterpret_cast()` but round-trips cannot occur the other
    //      way).
    //
    // Example:
    //
    //   float f = 3.14159265358979;
    //   int i = bit_cast<int32_t>(f);
    //   // i = 0x40490fdb
    //
    // Casting non-pointer types to pointer types and then dereferencing them
    // traditionally produces undefined behavior.
    //
    // Example:
    //
    //   // WRONG
    //   float f = 3.14159265358979;            // WRONG
    //   int i = * reinterpret_cast<int*>(&f);  // WRONG
    //
    // The address-casting method produces undefined behavior according to the ISO
    // C++ specification section [basic.lval]. Roughly, this section says: if an
    // object in memory has one type, and a program accesses it with a different
    // type, the result is undefined behavior for most values of "different type".
    //
    // Such casting results in type punning: holding an object in memory of one type
    // and reading its bits back using a different type. A `bit_cast()` avoids this
    // issue by implementing its casts using `memcpy()`, which avoids introducing
    // this undefined behavior.
    //
    // NOTE: The requirements here are more strict than the bit_cast of standard
    // proposal p0476 due to the need for workarounds and lack of intrinsics.
    // Specifically, this implementation also requires `Dest` to be
    // default-constructible.
    template<
            typename Dest, typename Source,
            typename std::enable_if<math_internal::is_bitcastable<Dest, Source>::value,
                    int>::type = 0>
    FLARE_FORCE_INLINE Dest bit_cast(const Source &source) {
        Dest dest;
        memcpy(static_cast<void *>(std::addressof(dest)),
               static_cast<const void *>(std::addressof(source)), sizeof(dest));
        return dest;
    }



}  // namespace flare::base

#endif  // FLARE_BASE_MATH_BIT_CAST_H_
