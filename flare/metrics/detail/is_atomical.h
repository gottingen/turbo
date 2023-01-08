
#ifndef FLARE_VARIABLE_DETAIL_IS_ATOMICAL_H_
#define FLARE_VARIABLE_DETAIL_IS_ATOMICAL_H_

#include "flare/base/type_traits.h"

namespace flare {
    namespace metrics_detail {
        template<class T>
        struct is_atomical
                : std::integral_constant<bool, (std::is_integral<T>::value ||
                                                std::is_floating_point<T>::value)> {
        };
        template<class T>
        struct is_atomical<const T> : is_atomical<T> {
        };
        template<class T>
        struct is_atomical<volatile T> : is_atomical<T> {
        };
        template<class T>
        struct is_atomical<const volatile T> : is_atomical<T> {
        };

    }  // namespace metrics_detail
}  // namespace flare

#endif
