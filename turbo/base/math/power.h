//
// Created by liyinbin on 2023/1/18.
//

#ifndef TURBO_BASE_MATH_POWER_H_
#define TURBO_BASE_MATH_POWER_H_

#include "turbo/base/math/constexpr_math.h"
#include "turbo/base/utility.h"

namespace turbo::base {

    namespace detail {
        template <typename Dst, typename Src>
        constexpr std::make_unsigned_t<Dst> bits_to_unsigned(Src const s) {
            static_assert(std::is_unsigned<Dst>::value, "signed type");
            return static_cast<Dst>(to_unsigned(s));
        }
    }  //  namespace detail


    /// find_last_set
    ///
    /// Return the 1-based index of the most significant bit which is set.
    /// For x > 0, find_last_set(x) == 1 + floor(log2(x)).
    template <typename T>
    inline constexpr unsigned int find_last_set(T const v) {
        using U0 = unsigned int;
        using U1 = unsigned long int;
        using U2 = unsigned long long int;
        using detail::bits_to_unsigned;
        static_assert(sizeof(T) <= sizeof(U2), "over-sized type");
        static_assert(std::is_integral<T>::value, "non-integral type");
        static_assert(!std::is_same<T, bool>::value, "bool type");

        // If X is a power of two X - Y = 1 + ((X - 1) ^ Y). Doing this transformation
        // allows GCC to remove its own xor that it adds to implement clz using bsr.
        // clang-format off
        using size = std::integral_constant<size_t, constexpr_max(sizeof(T), sizeof(U0))>;
        return v ? 1u + static_cast<unsigned int>((8u * size{} - 1u) ^ (
                sizeof(T) <= sizeof(U0) ? __builtin_clz(bits_to_unsigned<U0>(v)) :
                sizeof(T) <= sizeof(U1) ? __builtin_clzl(bits_to_unsigned<U1>(v)) :
                sizeof(T) <= sizeof(U2) ? __builtin_clzll(bits_to_unsigned<U2>(v)) :
                0)) : 0u;
        // clang-format on
    }


    template <class T>
    inline constexpr T next_pow_two(T const v) {
        static_assert(std::is_unsigned<T>::value, "signed type");
        return v ? (T(1) << find_last_set(v - 1)) : T(1);
    }

    template <class T>
    inline constexpr T prev_pow_two(T const v) {
        static_assert(std::is_unsigned<T>::value, "signed type");
        return v ? (T(1) << (find_last_set(v) - 1)) : T(0);
    }
}  // namespace turbo::base

#endif  // TURBO_BASE_MATH_POWER_H_
