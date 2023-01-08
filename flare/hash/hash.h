
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_HASH_HASH_H_
#define FLARE_HASH_HASH_H_

#include <cstdint>
#include <functional>
#include <tuple>
#include <flare/base/int128.h>

namespace flare {

    template<int n>
    struct flare_mix {
        inline size_t operator()(size_t) const;
    };

    template<>
    struct flare_mix<4> {
        inline size_t operator()(size_t a) const {
            static constexpr uint64_t kmul = 0xcc9e2d51UL;
            // static constexpr uint64_t kmul = 0x3B9ACB93UL; // [greg] my own random prime
            uint64_t l = a * kmul;
            return static_cast<size_t>(l ^ (l >> 32));
        }
    };

    template<>
    struct flare_mix<8> {
        // Very fast mixing (similar to Abseil)
        inline size_t operator()(uint128 a) const {
            static constexpr uint128 k = 0xde5fb9d2630458e9ULL;
            // static constexpr uint64_t k = 0x7C9D0BF0567102A5ULL; // [greg] my own random prime
            uint128 hh = a * k;
            return uint128_high64(hh) + uint128_low64(hh);
        }
    };


    // --------------------------------------------
    template<int n>
    struct fold_if_needed {
        inline size_t operator()(uint64_t) const;
    };

    template<>
    struct fold_if_needed<4> {
        inline size_t operator()(uint64_t a) const {
            return static_cast<size_t>(a ^ (a >> 32));
        }
    };

    template<>
    struct fold_if_needed<8> {
        inline size_t operator()(uint64_t a) const {
            return static_cast<size_t>(a);
        }
    };

    // ---------------------------------------------------------------
    // see if class T has a hash_value() friend method
    // ---------------------------------------------------------------
    template<typename T>
    struct has_hash_value {
    private:
        typedef std::true_type yes;
        typedef std::false_type no;

        template<typename U>
        static auto test(int) -> decltype(hash_value(std::declval<const U &>()) == 1, yes());

        template<typename>
        static no test(...);

    public:
        static constexpr bool value = std::is_same<decltype(test<T>(0)), yes>::value;
    };

    template<class T>
    struct hash {
        template<class U, typename std::enable_if<has_hash_value<U>::value, int>::type = 0>
        size_t hash_impl(const T &val) const {
            return hash_value(val);
        }

        template<class U, typename std::enable_if<!has_hash_value<U>::value, int>::type = 0>
        size_t hash_impl(const T &val) const {
            return std::hash<T>()(val);
        }

        inline size_t operator()(const T &val) const {
            return hash_impl<T>(val);
        }
    };

    template<class T>
    struct hash<T *> {
        inline size_t operator()(const T *val) const noexcept {
            return static_cast<size_t>(reinterpret_cast<const uintptr_t>(val));
        }
    };

    template<class ArgumentType, class ResultType>
    struct flare_unary_function {
        typedef ArgumentType argument_type;
        typedef ResultType result_type;
    };

    template<>
    struct hash<bool> : public flare_unary_function<bool, size_t> {
        inline size_t operator()(bool val) const noexcept { return static_cast<size_t>(val); }
    };

    template<>
    struct hash<char> : public flare_unary_function<char, size_t> {
        inline size_t operator()(char val) const noexcept { return static_cast<size_t>(val); }
    };

    template<>
    struct hash<signed char> : public flare_unary_function<signed char, size_t> {
        inline size_t operator()(signed char val) const noexcept { return static_cast<size_t>(val); }
    };

    template<>
    struct hash<unsigned char> : public flare_unary_function<unsigned char, size_t> {
        inline size_t operator()(unsigned char val) const noexcept { return static_cast<size_t>(val); }
    };

    template<>
    struct hash<int16_t> : public flare_unary_function<int16_t, size_t> {
        inline size_t operator()(int16_t val) const noexcept { return static_cast<size_t>(val); }
    };

    template<>
    struct hash<uint16_t> : public flare_unary_function<uint16_t, size_t> {
        inline size_t operator()(uint16_t val) const noexcept { return static_cast<size_t>(val); }
    };

    template<>
    struct hash<int32_t> : public flare_unary_function<int32_t, size_t> {
        inline size_t operator()(int32_t val) const noexcept { return static_cast<size_t>(val); }
    };

    template<>
    struct hash<uint32_t> : public flare_unary_function<uint32_t, size_t> {
        inline size_t operator()(uint32_t val) const noexcept { return static_cast<size_t>(val); }
    };

    template<>
    struct hash<int64_t> : public flare_unary_function<int64_t, size_t> {
        inline size_t operator()(int64_t val) const noexcept {
            return fold_if_needed<sizeof(size_t)>()(static_cast<uint64_t>(val));
        }
    };

    template<>
    struct hash<uint64_t> : public flare_unary_function<uint64_t, size_t> {
        inline size_t operator()(uint64_t val) const noexcept { return fold_if_needed<sizeof(size_t)>()(val); }
    };

    template<>
    struct hash<float> : public flare_unary_function<float, size_t> {
        inline size_t operator()(float val) const noexcept {
            // -0.0 and 0.0 should return same hash
            uint32_t *as_int = reinterpret_cast<uint32_t *>(&val);
            return (val == 0) ? static_cast<size_t>(0) :
                   static_cast<size_t>(*as_int);
        }
    };

    template<>
    struct hash<double> : public flare_unary_function<double, size_t> {
        inline size_t operator()(double val) const noexcept {
            // -0.0 and 0.0 should return same hash
            uint64_t *as_int = reinterpret_cast<uint64_t *>(&val);
            return (val == 0) ? static_cast<size_t>(0) :
                   fold_if_needed<sizeof(size_t)>()(*as_int);
        }
    };


    template<class H, int sz>
    struct hash_combiner {
        H operator()(H seed, size_t value);
    };

    template<class H>
    struct hash_combiner<H, 4> {
        H operator()(H seed, size_t value) {
            return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
        }
    };

    template<class H>
    struct hash_combiner<H, 8> {
        H operator()(H seed, size_t value) {
            return seed ^ (value + size_t(0xc6a4a7935bd1e995) + (seed << 6) + (seed >> 2));
        }
    };

    // define hash_state to combine member hashes... see example below
    // -----------------------------------------------------------------------------
    template<typename H>
    class hash_state_base {
    public:
        template<typename T, typename... Ts>
        static H combine(H state, const T &value, const Ts &... values);

        static H combine(H state) { return state; }
    };

    template<typename H>
    template<typename T, typename... Ts>
    H hash_state_base<H>::combine(H seed, const T &v, const Ts &... vs) {
        return hash_state_base<H>::combine(hash_combiner<H, sizeof(H)>()(seed, flare::hash<T>()(v)), vs...);
    }

    using hash_state = hash_state_base<size_t>;


    // define hash for std::pair
    // -------------------------
    template<class T1, class T2>
    struct hash<std::pair<T1, T2>> {
        size_t operator()(std::pair<T1, T2> const &p) const noexcept {
            return flare::hash_state().combine(flare::hash<T1>()(p.first), p.second);
        }
    };

    // define hash for std::tuple
    // --------------------------
    template<class... T>
    struct hash<std::tuple<T...>> {
        size_t operator()(std::tuple<T...> const &t) const noexcept {
            return hash_impl_helper(t);
        }

    private:

        template<size_t I = 0, class ...P>
        typename std::enable_if<I == sizeof...(P), size_t>::type
        hash_impl_helper(const std::tuple<P...> &) const noexcept { return 0; }

        template<size_t I = 0, class ...P>
        typename std::enable_if<I < sizeof...(P), size_t>::type
        hash_impl_helper(const std::tuple<P...> &t) const noexcept {
            const auto &el = std::get<I>(t);
            using el_type = typename std::remove_cv<typename std::remove_reference<decltype(el)>::type>::type;
            return hash_combiner<size_t, sizeof(size_t)>()(
                    flare::hash<el_type>()(el), hash_impl_helper<I + 1>(t));
        }
    };

}  // namespace flare

#endif // FLARE_HASH_HASH_H_
