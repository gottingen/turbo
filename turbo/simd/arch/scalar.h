// Copyright 2023 The titan-search Authors.
// Copyright (c) Jeff.li
// Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and Martin Renou
// Copyright (c) QuantStack
// Copyright (c) Serge Guelton
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#ifndef TURBO_SIMD_ARCH_SCALAR_H_
#define TURBO_SIMD_ARCH_SCALAR_H_

#include <cassert>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace turbo::simd {
    template<class T, class A>
    class batch;

    template<class T, class A>
    class batch_bool;

    using std::abs;

    using std::acos;
    using std::acosh;
    using std::arg;
    using std::asin;
    using std::asinh;
    using std::atan;
    using std::atan2;
    using std::atanh;
    using std::cbrt;
    using std::ceil;
    using std::conj;
    using std::copysign;
    using std::cos;
    using std::cosh;
    using std::erf;
    using std::erfc;
    using std::exp;
    using std::exp2;
    using std::expm1;
    using std::fabs;
    using std::fdim;
    using std::floor;
    using std::fmax;
    using std::fmin;
    using std::fmod;
    using std::hypot;
    using std::ldexp;
    using std::lgamma;
    using std::log;
    using std::log10;
    using std::log1p;
    using std::log2;
    using std::modf;
    using std::nearbyint;
    using std::nextafter;
    using std::norm;
    using std::polar;
    using std::proj;
    using std::remainder;
    using std::rint;
    using std::round;
    using std::sin;
    using std::sinh;
    using std::sqrt;
    using std::tan;
    using std::tanh;
    using std::tgamma;
    using std::trunc;

#ifndef _WIN32
    using std::isfinite;
    using std::isinf;
    using std::isnan;
#else

    // Windows defines catch all templates
    template <class T>
    inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    isfinite(T var) noexcept
    {
        return std::isfinite(var);
    }

    template <class T>
    inline typename std::enable_if<std::is_integral<T>::value, bool>::type
    isfinite(T var) noexcept
    {
        return isfinite(double(var));
    }

    template <class T>
    inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    isinf(T var) noexcept
    {
        return std::isinf(var);
    }

    template <class T>
    inline typename std::enable_if<std::is_integral<T>::value, bool>::type
    isinf(T var) noexcept
    {
        return isinf(double(var));
    }

    template <class T>
    inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    isnan(T var) noexcept
    {
        return std::isnan(var);
    }

    template <class T>
    inline typename std::enable_if<std::is_integral<T>::value, bool>::type
    isnan(T var) noexcept
    {
        return isnan(double(var));
    }
#endif

    template<class T, class Tp>
    inline auto add(T const &x, Tp const &y) noexcept -> decltype(x + y) {
        return x + y;
    }

    template<class T>
    inline T incr(T const &x) noexcept {
        return x + T(1);
    }

    template<class T>
    inline T incr_if(T const &x, bool mask) noexcept {
        return x + T(mask ? 1 : 0);
    }

    inline bool all(bool mask) {
        return mask;
    }

    inline bool any(bool mask) {
        return mask;
    }

    inline bool none(bool mask) {
        return !mask;
    }

    template<class T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    bitwise_and(T x, T y) noexcept {
        return x & y;
    }

    inline float bitwise_and(float x, float y) noexcept {
        uint32_t ix, iy;
        std::memcpy((void *) &ix, (void *) &x, sizeof(float));
        std::memcpy((void *) &iy, (void *) &y, sizeof(float));
        uint32_t ir = bitwise_and(ix, iy);
        float r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(float));
        return r;
    }

    inline double bitwise_and(double x, double y) noexcept {
        uint64_t ix, iy;
        std::memcpy((void *) &ix, (void *) &x, sizeof(double));
        std::memcpy((void *) &iy, (void *) &y, sizeof(double));
        uint64_t ir = bitwise_and(ix, iy);
        double r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(double));
        return r;
    }

    template<class T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    bitwise_andnot(T x, T y) noexcept {
        return x & ~y;
    }

    inline float bitwise_andnot(float x, float y) noexcept {
        uint32_t ix, iy;
        std::memcpy((void *) &ix, (void *) &x, sizeof(float));
        std::memcpy((void *) &iy, (void *) &y, sizeof(float));
        uint32_t ir = bitwise_andnot(ix, iy);
        float r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(float));
        return r;
    }

    inline double bitwise_andnot(double x, double y) noexcept {
        uint64_t ix, iy;
        std::memcpy((void *) &ix, (void *) &x, sizeof(double));
        std::memcpy((void *) &iy, (void *) &y, sizeof(double));
        uint64_t ir = bitwise_andnot(ix, iy);
        double r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(double));
        return r;
    }

    template<class T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    bitwise_not(T x) noexcept {
        return ~x;
    }

    inline float bitwise_not(float x) noexcept {
        uint32_t ix;
        std::memcpy((void *) &ix, (void *) &x, sizeof(float));
        uint32_t ir = bitwise_not(ix);
        float r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(float));
        return r;
    }

    inline double bitwise_not(double x) noexcept {
        uint64_t ix;
        std::memcpy((void *) &ix, (void *) &x, sizeof(double));
        uint64_t ir = bitwise_not(ix);
        double r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(double));
        return r;
    }

    template<class T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    bitwise_or(T x, T y) noexcept {
        return x | y;
    }

    inline float bitwise_or(float x, float y) noexcept {
        uint32_t ix, iy;
        std::memcpy((void *) &ix, (void *) &x, sizeof(float));
        std::memcpy((void *) &iy, (void *) &y, sizeof(float));
        uint32_t ir = bitwise_or(ix, iy);
        float r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(float));
        return r;
    }

    inline double bitwise_or(double x, double y) noexcept {
        uint64_t ix, iy;
        std::memcpy((void *) &ix, (void *) &x, sizeof(double));
        std::memcpy((void *) &iy, (void *) &y, sizeof(double));
        uint64_t ir = bitwise_or(ix, iy);
        double r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(double));
        return r;
    }

    template<class T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    bitwise_xor(T x, T y) noexcept {
        return x ^ y;
    }

    inline float bitwise_xor(float x, float y) noexcept {
        uint32_t ix, iy;
        std::memcpy((void *) &ix, (void *) &x, sizeof(float));
        std::memcpy((void *) &iy, (void *) &y, sizeof(float));
        uint32_t ir = bitwise_xor(ix, iy);
        float r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(float));
        return r;
    }

    inline double bitwise_xor(double x, double y) noexcept {
        uint64_t ix, iy;
        std::memcpy((void *) &ix, (void *) &x, sizeof(double));
        std::memcpy((void *) &iy, (void *) &y, sizeof(double));
        uint64_t ir = bitwise_xor(ix, iy);
        double r;
        std::memcpy((void *) &r, (void *) &ir, sizeof(double));
        return r;
    }

    template<class T, class Tp>
    inline auto div(T const &x, Tp const &y) noexcept -> decltype(x / y) {
        return x / y;
    }

    template<class T, class Tp>
    inline auto mod(T const &x, Tp const &y) noexcept -> decltype(x % y) {
        return x % y;
    }

    template<class T, class Tp>
    inline auto mul(T const &x, Tp const &y) noexcept -> decltype(x * y) {
        return x * y;
    }

    template<class T>
    inline auto neg(T const &x) noexcept -> decltype(-x) {
        return -x;
    }

    template<class T>
    inline auto pos(T const &x) noexcept -> decltype(+x) {
        return +x;
    }

    inline float reciprocal(float const &x) noexcept {
        return 1.f / x;
    }

    inline double reciprocal(double const &x) noexcept {
        return 1. / x;
    }

#ifdef TURBO_SIMD_ENABLE_NUMPY_COMPLEX
    template <class T>
    inline bool isnan(std::complex<T> var) noexcept
    {
        return std::isnan(std::real(var)) || std::isnan(std::imag(var));
    }

    template <class T>
    inline bool isinf(std::complex<T> var) noexcept
    {
        return std::isinf(std::real(var)) || std::isinf(std::imag(var));
    }
#endif


    template<typename T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T clip(const T &val, const T &low, const T &hi) noexcept {
        assert(low <= hi && "ordered clipping bounds");
        return low > val ? low : (hi < val ? hi : val);
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool is_flint(const T &x) noexcept {
        return std::isnan(x - x) ? false : (x - std::trunc(x)) == T(0);
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool is_even(const T &x) noexcept {
        return is_flint(x * T(0.5));
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool is_odd(const T &x) noexcept {
        return is_even(x - 1.);
    }

    inline int32_t nearbyint_as_int(float var) noexcept {
        return static_cast<int32_t>(std::nearbyint(var));
    }

    inline int64_t nearbyint_as_int(double var) noexcept {
        return static_cast<int64_t>(std::nearbyint(var));
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool eq(const T &x0, const T &x1) noexcept {
        return x0 == x1;
    }

    template<class T>
    inline bool eq(const std::complex<T> &x0, const std::complex<T> &x1) noexcept {
        return x0 == x1;
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool ge(const T &x0, const T &x1) noexcept {
        return x0 >= x1;
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool gt(const T &x0, const T &x1) noexcept {
        return x0 > x1;
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool le(const T &x0, const T &x1) noexcept {
        return x0 <= x1;
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool lt(const T &x0, const T &x1) noexcept {
        return x0 < x1;
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool neq(const T &x0, const T &x1) noexcept {
        return x0 != x1;
    }

    template<class T>
    inline bool neq(const std::complex<T> &x0, const std::complex<T> &x1) noexcept {
        return !(x0 == x1);
    }

#if defined(__APPLE__)
    inline float exp10(const float& x) noexcept
    {
        return __exp10f(x);
    }
    inline double exp10(const double& x) noexcept
    {
        return __exp10(x);
    }
#elif defined(__GLIBC__)

    inline float exp10(const float &x) noexcept {
        return ::exp10f(x);
    }

    inline double exp10(const double &x) noexcept {
        return ::exp10(x);
    }

#elif defined(_WIN32)
    template <class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T exp10(const T& x) noexcept
    {
        // Very inefficient but other implementations give incorrect results
        // on Windows
        return std::pow(T(10), x);
    }
#else
    inline float exp10(const float& x) noexcept
    {
        const float ln10 = std::log(10.f);
        return std::exp(ln10 * x);
    }
    inline double exp10(const double& x) noexcept
    {
        const double ln10 = std::log(10.);
        return std::exp(ln10 * x);
    }
#endif

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline auto rsqrt(const T &x) noexcept -> decltype(std::sqrt(x)) {
        using float_type = decltype(std::sqrt(x));
        return static_cast<float_type>(1) / std::sqrt(x);
    }

    namespace detail {
        template<class C>
        inline C expm1_complex_scalar_impl(const C &val) noexcept {
            using T = typename C::value_type;
            T isin = std::sin(val.imag());
            T rem1 = std::expm1(val.real());
            T re = rem1 + T(1.);
            T si = std::sin(val.imag() * T(0.5));
            return std::complex<T>(rem1 - T(2.) * re * si * si, re * isin);
        }
    }

    template<class T>
    inline std::complex<T> expm1(const std::complex<T> &val) noexcept {
        return detail::expm1_complex_scalar_impl(val);
    }

    namespace detail {
        template<class C>
        inline C log1p_complex_scalar_impl(const C &val) noexcept {
            using T = typename C::value_type;
            C u = C(1.) + val;
            return u == C(1.) ? val : (u.real() <= T(0.) ? log(u) : log(u) * val / (u - C(1.)));
        }
    }

    template<class T>
    inline std::complex<T> log1p(const std::complex<T> &val) noexcept {
        return detail::log1p_complex_scalar_impl(val);
    }

    template<class T>
    inline std::complex<T> log2(const std::complex<T> &val) noexcept {
        return log(val) / std::log(T(2));
    }

    template<typename T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T sadd(const T &lhs, const T &rhs) noexcept {
        if (std::numeric_limits<T>::is_signed) {
            if ((lhs > 0) && (rhs > std::numeric_limits<T>::max() - lhs)) {
                return std::numeric_limits<T>::max();
            } else if ((lhs < 0) && (rhs < std::numeric_limits<T>::lowest() - lhs)) {
                return std::numeric_limits<T>::lowest();
            } else {
                return lhs + rhs;
            }
        } else {
            if (rhs > std::numeric_limits<T>::max() - lhs) {
                return std::numeric_limits<T>::max();
            } else {
                return lhs + rhs;
            }
        }
    }

    template<typename T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T ssub(const T &lhs, const T &rhs) noexcept {
        if (std::numeric_limits<T>::is_signed) {
            return sadd(lhs, (T) -rhs);
        } else {
            if (lhs < rhs) {
                return std::numeric_limits<T>::lowest();
            } else {
                return lhs - rhs;
            }
        }
    }

    namespace detail {
        template<class T>
        struct value_type_or_type_helper {
            using type = T;
        };
        template<class T, class A>
        struct value_type_or_type_helper<batch<T, A>> {
            using type = T;
        };

        template<class T>
        using value_type_or_type = typename value_type_or_type_helper<T>::type;

        template<class T0, class T1>
        inline typename std::enable_if<std::is_integral<T1>::value, T0>::type
        ipow(const T0 &x, const T1 &n) noexcept {
            static_assert(std::is_integral<T1>::value, "second argument must be an integer");
            T0 a = x;
            T1 b = n;
            bool const recip = b < 0;
            T0 r(static_cast<value_type_or_type<T0>>(1));
            while (1) {
                if (b & 1) {
                    r *= a;
                }
                b /= 2;
                if (b == 0) {
                    break;
                }
                a *= a;
            }
            return recip ? static_cast<T0>(1) / r : r;
        }
    }

    template<class T0, class T1>
    inline typename std::enable_if<std::is_integral<T1>::value, T0>::type
    pow(const T0 &x, const T1 &n) noexcept {
        return detail::ipow(x, n);
    }

    template<class T0, class T1>
    inline auto
    pow(const T0 &t0, const T1 &t1) noexcept
    -> typename std::enable_if<
            std::is_scalar<T0>::value && std::is_floating_point<T1>::value, decltype(std::pow(t0, t1))>::type {
        return std::pow(t0, t1);
    }

    template<class T0, class T1>
    inline typename std::enable_if<std::is_integral<T1>::value, std::complex<T0>>::type
    pow(const std::complex<T0> &t0, const T1 &t1) noexcept {
        return detail::ipow(t0, t1);
    }

    template<class T0, class T1>
    inline typename std::enable_if<!std::is_integral<T1>::value, std::complex<T0>>::type
    pow(const std::complex<T0> &t0, const T1 &t1) noexcept {
        return std::pow(t0, t1);
    }

    template<class T0, class T1>
    inline auto
    pow(const T0 &t0, const std::complex<T1> &t1) noexcept
    -> typename std::enable_if<std::is_scalar<T0>::value, decltype(std::pow(t0, t1))>::type {
        return std::pow(t0, t1);
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool bitofsign(T const &x) noexcept {
        return x < T(0);
    }

    template<class T>
    inline auto signbit(T const &v) noexcept -> decltype(bitofsign(v)) {
        return bitofsign(v);
    }

    inline double sign(bool const &v) noexcept {
        return v;
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T sign(const T &v) noexcept {
        return v < T(0) ? T(-1.) : v == T(0) ? T(0.)
                                             : T(1.);
    }

    namespace detail {
        template<class C>
        inline C sign_complex_scalar_impl(const C &v) noexcept {
            using value_type = typename C::value_type;
            if (v.real()) {
                return C(sign(v.real()), value_type(0));
            } else {
                return C(sign(v.imag()), value_type(0));
            }
        }
    }

    template<class T>
    inline std::complex<T> sign(const std::complex<T> &v) noexcept {
        return detail::sign_complex_scalar_impl(v);
    }


    inline double signnz(bool const &) noexcept {
        return 1;
    }

    template<class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T signnz(const T &v) noexcept {
        return v < T(0) ? T(-1.) : T(1.);
    }

    template<class T, class Tp>
    inline auto sub(T const &x, Tp const &y) noexcept -> decltype(x - y) {
        return x - y;
    }

    template<class T>
    inline T decr(T const &x) noexcept {
        return x - T(1);
    }

    template<class T>
    inline T decr_if(T const &x, bool mask) noexcept {
        return x - T(mask ? 1 : 0);
    }

    template<class T0, class T1>
    inline auto min(T0 const &self, T1 const &other) noexcept
    -> typename std::enable_if<std::is_scalar<T0>::value && std::is_scalar<T1>::value,
            typename std::decay<decltype(self > other ? other : self)>::type>::type {
        return self > other ? other : self;
    }

    // numpy defines minimum operator on complex using lexical comparison
    template<class T0, class T1>
    inline std::complex<typename std::common_type<T0, T1>::type>
    min(std::complex<T0> const &self, std::complex<T1> const &other) noexcept {
        return (self.real() < other.real()) ? (self) : (self.real() == other.real() ? (self.imag() < other.imag() ? self
                                                                                                                  : other)
                                                                                    : other);
    }

    template<class T0, class T1>
    inline auto max(T0 const &self, T1 const &other) noexcept
    -> typename std::enable_if<std::is_scalar<T0>::value && std::is_scalar<T1>::value,
            typename std::decay<decltype(self > other ? other : self)>::type>::type {
        return self < other ? other : self;
    }

    // numpy defines maximum operator on complex using lexical comparison
    template<class T0, class T1>
    inline std::complex<typename std::common_type<T0, T1>::type>
    max(std::complex<T0> const &self, std::complex<T1> const &other) noexcept {
        return (self.real() > other.real()) ? (self) : (self.real() == other.real() ? (self.imag() > other.imag() ? self
                                                                                                                  : other)
                                                                                    : other);
    }

    template<class T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    fma(const T &a, const T &b, const T &c) noexcept {
        return a * b + c;
    }

    template<class T>
    inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
    fma(const T &a, const T &b, const T &c) noexcept {
        return std::fma(a, b, c);
    }

    template<class T>
    inline typename std::enable_if<std::is_scalar<T>::value, T>::type fms(const T &a, const T &b, const T &c) noexcept {
        return a * b - c;
    }

    namespace detail {
        template<class C>
        inline C fma_complex_scalar_impl(const C &a, const C &b, const C &c) noexcept {
            return {fms(a.real(), b.real(), fms(a.imag(), b.imag(), c.real())),
                    fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag()))};
        }
    }

    template<class T>
    inline std::complex<T> fma(const std::complex<T> &a, const std::complex<T> &b, const std::complex<T> &c) noexcept {
        return detail::fma_complex_scalar_impl(a, b, c);
    }


    namespace detail {
        template<class C>
        inline C fms_complex_scalar_impl(const C &a, const C &b, const C &c) noexcept {
            return {fms(a.real(), b.real(), fma(a.imag(), b.imag(), c.real())),
                    fma(a.real(), b.imag(), fms(a.imag(), b.real(), c.imag()))};
        }
    }

    template<class T>
    inline std::complex<T> fms(const std::complex<T> &a, const std::complex<T> &b, const std::complex<T> &c) noexcept {
        return detail::fms_complex_scalar_impl(a, b, c);
    }

    template<class T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    fnma(const T &a, const T &b, const T &c) noexcept {
        return -(a * b) + c;
    }

    template<class T>
    inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
    fnma(const T &a, const T &b, const T &c) noexcept {
        return std::fma(-a, b, c);
    }

    namespace detail {
        template<class C>
        inline C fnma_complex_scalar_impl(const C &a, const C &b, const C &c) noexcept {
            return {fms(a.imag(), b.imag(), fms(a.real(), b.real(), c.real())),
                    -fma(a.real(), b.imag(), fms(a.imag(), b.real(), c.imag()))};
        }
    }

    template<class T>
    inline std::complex<T> fnma(const std::complex<T> &a, const std::complex<T> &b, const std::complex<T> &c) noexcept {
        return detail::fnma_complex_scalar_impl(a, b, c);
    }


    template<class T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    fnms(const T &a, const T &b, const T &c) noexcept {
        return -(a * b) - c;
    }

    template<class T>
    inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
    fnms(const T &a, const T &b, const T &c) noexcept {
        return -std::fma(a, b, c);
    }

    namespace detail {
        template<class C>
        inline C fnms_complex_scalar_impl(const C &a, const C &b, const C &c) noexcept {
            return {fms(a.imag(), b.imag(), fma(a.real(), b.real(), c.real())),
                    -fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag()))};
        }
    }

    template<class T>
    inline std::complex<T> fnms(const std::complex<T> &a, const std::complex<T> &b, const std::complex<T> &c) noexcept {
        return detail::fnms_complex_scalar_impl(a, b, c);
    }

    namespace detail {
#define TURBO_SIMD_HAS_SIN_COS_TRAIT(func)                                                                                              \
    template <class S>                                                                                                           \
    struct has##func                                                                                                             \
    {                                                                                                                            \
        template <class T>                                                                                                       \
        static auto get(T* ptr) -> decltype(func(std::declval<T>(), std::declval<T*>(), std::declval<T*>()), std::true_type {}); \
        static std::false_type get(...);                                                                                         \
        static constexpr bool value = decltype(get((S*)nullptr))::value;                                                         \
    }

#define TURBO_SIMD_HAS_SIN_COS(func, T) has##func<T>::value

        TURBO_SIMD_HAS_SIN_COS_TRAIT(sincos);

        TURBO_SIMD_HAS_SIN_COS_TRAIT(sincosf);

        TURBO_SIMD_HAS_SIN_COS_TRAIT(__sincos);

        TURBO_SIMD_HAS_SIN_COS_TRAIT(__sincosf);

        struct generic_sincosf {
            template<class T>
            typename std::enable_if<TURBO_SIMD_HAS_SIN_COS(sincosf, T), void>::type
            operator()(float val, T &s, T &c) {
                sincosf(val, &s, &c);
            }

            template<class T>
            typename std::enable_if<!TURBO_SIMD_HAS_SIN_COS(sincosf, T) && TURBO_SIMD_HAS_SIN_COS(__sincosf, T), void>::type
            operator()(float val, T &s, T &c) {
                __sincosf(val, &s, &c);
            }

            template<class T>
            typename std::enable_if<!TURBO_SIMD_HAS_SIN_COS(sincosf, T) && !TURBO_SIMD_HAS_SIN_COS(__sincosf, T), void>::type
            operator()(float val, T &s, T &c) {
                s = std::sin(val);
                c = std::cos(val);
            }
        };

        struct generic_sincos {
            template<class T>
            typename std::enable_if<TURBO_SIMD_HAS_SIN_COS(sincos, T), void>::type
            operator()(double val, T &s, T &c) {
                sincos(val, &s, &c);
            }

            template<class T>
            typename std::enable_if<!TURBO_SIMD_HAS_SIN_COS(sincos, T) && TURBO_SIMD_HAS_SIN_COS(__sincos, T), void>::type
            operator()(double val, T &s, T &c) {
                __sincos(val, &s, &c);
            }

            template<class T>
            typename std::enable_if<!TURBO_SIMD_HAS_SIN_COS(sincos, T) && !TURBO_SIMD_HAS_SIN_COS(__sincos, T), void>::type
            operator()(double val, T &s, T &c) {
                s = std::sin(val);
                c = std::cos(val);
            }
        };

#undef TURBO_SIMD_HAS_SIN_COS_TRAIT
#undef TURBO_SIMD_HAS_SIN_COS
    }

    inline std::pair<float, float> sincos(float val) noexcept {
        float s, c;
        detail::generic_sincosf{}(val, s, c);
        return std::make_pair(s, c);
    }

    inline std::pair<double, double> sincos(double val) noexcept {
        double s, c;
        detail::generic_sincos{}(val, s, c);
        return std::make_pair(s, c);
    }

    template<class T>
    inline std::pair<std::complex<T>, std::complex<T>>
    sincos(const std::complex<T> &val) noexcept {
        return std::make_pair(std::sin(val), std::cos(val));
    }


    template<class T, class _ = typename std::enable_if<std::is_floating_point<T>::value, void>::type>
    inline T frexp(T const &val, int &exp) noexcept {
        return std::frexp(val, &exp);
    }

    template<class T>
    inline T select(bool cond, T const &true_br, T const &false_br) noexcept {
        return cond ? true_br : false_br;
    }

}

#endif  // TURBO_SIMD_ARCH_SCALAR_H_

