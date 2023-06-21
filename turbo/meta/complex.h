// Copyright 2023 The titan-search Authors.
//
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
#ifndef TURBO_META_COMPLEX_H_
#define TURBO_META_COMPLEX_H_

#if !defined(_MSC_VER)

#include <cmath>

using std::copysign;
#endif

#include <complex>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>
#include <sstream>
#include <string>
#include "turbo/meta/closure.h"
#include "turbo/meta/type_traits.h"

namespace turbo {
    template<class CTR, class CTI = CTR, bool ieee_compliant = false>
    class complex;

    /**************
     * is_std_complex *
     **************/

    namespace detail {
        template<class T>
        struct is_std_complex : std::false_type {
        };

        template<class T>
        struct is_std_complex<std::complex<T>> : std::true_type {
        };
    }

    template<class T>
    struct is_std_complex {
        static constexpr bool value = detail::is_std_complex<std::decay_t<T>>::value;
    };

    /***************
     * is_complex *
     ***************/

    namespace detail {
        template<class T>
        struct is_complex : std::false_type {
        };

        template<class CTR, class CTI, bool B>
        struct is_complex<complex<CTR, CTI, B>> : std::true_type {
        };
    }

    template<class T>
    struct is_complex {
        static constexpr bool value = detail::is_complex<std::decay_t<T>>::value;
    };

    /******************
     * is_gen_complex *
     ******************/

    template<class T>
    using is_gen_complex = disjunction<is_std_complex<std::decay_t<T>>, is_complex<std::decay_t<T>>>;

    /****************************
     * enable / disable complex *
     ****************************/

    template<class E, class R = void>
    using disable_complex = std::enable_if_t<!is_gen_complex<E>::value, R>;

    template<class E, class R = void>
    using enable_complex = std::enable_if_t<is_gen_complex<E>::value, R>;

    /*****************
     * enable_scalar *
     *****************/

    template<class E, class R = void>
    using enable_scalar = std::enable_if_t<std::is_arithmetic<E>::value, R>;

    /*******************
     * common_complex *
     *******************/

    template<class CTR1, class CTI1, bool ieee1, class CTR2, class CTI2, bool ieee2>
    struct common_complex {
        using type = complex<std::common_type_t<CTR1, CTI1>, std::common_type_t<CTR2, CTI2>, ieee1 || ieee2>;
    };

    template<class CTR1, class CTI1, bool ieee1, class CTR2, class CTI2, bool ieee2>
    using common_complex_t = typename common_complex<CTR1, CTI1, ieee1, CTR2, CTI2, ieee2>::type;

    /**********************
     * temporary_complex *
     **********************/

    template<class CTR, class CTI, bool ieee>
    struct temporary_complex {
        using type = complex<std::decay_t<CTR>, std::decay_t<CTI>, ieee>;
    };

    template<class CTR, class CTI, bool ieee>
    using temporary_complex_t = typename temporary_complex<CTR, CTI, ieee>::type;

    /************
     * complex *
     ************/

    template<class CTR, class CTI, bool ieee_compliant>
    class complex {
    public:

        static_assert(std::is_same<std::decay_t<CTR>, std::decay_t<CTI>>::value,
                      "closure types must have the same value type");

        using value_type = std::common_type_t<CTR, CTI>;
        using self_type = complex<CTR, CTI, ieee_compliant>;
        using temporary_type = temporary_complex_t<CTR, CTI, ieee_compliant>;

        using real_reference = std::add_lvalue_reference_t<CTR>;
        using real_const_reference = std::add_lvalue_reference_t<std::add_const_t<CTR>>;
        using real_rvalue_reference = std::conditional_t<std::is_reference<CTR>::value, apply_cv_t<CTR, value_type> &, value_type>;
        using real_rvalue_const_reference = std::conditional_t<std::is_reference<CTR>::value, const value_type &, value_type>;

        using imag_reference = std::add_lvalue_reference_t<CTI>;
        using imag_const_reference = std::add_lvalue_reference_t<std::add_const_t<CTI>>;
        using imag_rvalue_reference = std::conditional_t<std::is_reference<CTI>::value, apply_cv_t<CTI, value_type> &, value_type>;
        using imag_rvalue_const_reference = std::conditional_t<std::is_reference<CTI>::value, const value_type &, value_type>;

        constexpr complex() noexcept
                : m_real(), m_imag() {
        }

        template<class OCTR,
                std::enable_if_t<
                        conjunction<
                                negation<is_gen_complex<OCTR>>,
                                std::is_constructible<CTR, OCTR &&>,
                                std::is_convertible<OCTR &&, CTR>
                        >::value,
                        bool
                > = true>
        constexpr complex(OCTR &&re) noexcept
                : m_real(std::forward<OCTR>(re)), m_imag() {
        }

        template<class OCTR,
                std::enable_if_t<
                        conjunction<
                                negation<is_gen_complex<OCTR>>,
                                std::is_constructible<CTR, OCTR &&>,
                                negation<std::is_convertible<OCTR &&, CTR>>
                        >::value,
                        bool
                > = true>
        explicit constexpr complex(OCTR &&re) noexcept
                : m_real(std::forward<OCTR>(re)), m_imag() {
        }

        template<class OCTR, class OCTI>
        explicit constexpr complex(OCTR &&re, OCTI &&im) noexcept
                : m_real(std::forward<OCTR>(re)), m_imag(std::forward<OCTI>(im)) {
        }

        template<class OCTR, class OCTI, bool OB>
        explicit constexpr complex(const complex<OCTR, OCTI, OB> &rhs) noexcept
                : m_real(rhs.real()), m_imag(rhs.imag()) {
        }

        template<class OCTR, class OCTI, bool OB>
        explicit constexpr complex(complex<OCTR, OCTI, OB> &&rhs) noexcept
                : m_real(std::move(rhs).real()), m_imag(std::move(rhs).imag()) {
        }

        template<class T>
        constexpr complex(const std::complex<T> &rhs) noexcept
                : m_real(rhs.real()), m_imag(rhs.imag()) {
        }

        template<class T>
        constexpr complex(std::complex<T> &&rhs) noexcept
                : m_real(std::move(rhs).real()), m_imag(std::move(rhs).imag()) {
        }

        template<class OCTR>
        disable_complex<OCTR, self_type &> operator=(OCTR &&rhs) noexcept;

        template<class OCTR, class OCTI, bool OB>
        self_type &operator=(const complex<OCTR, OCTI, OB> &rhs) noexcept;

        template<class OCTR, class OCTI, bool OB>
        self_type &operator=(complex<OCTR, OCTI, OB> &&rhs) noexcept;

        operator std::complex<std::decay_t<CTR>>() const noexcept;

        template<class OCTR, class OCTI, bool OB>
        self_type &operator+=(const complex<OCTR, OCTI, OB> &rhs) noexcept;

        template<class OCTR, class OCTI, bool OB>
        self_type &operator-=(const complex<OCTR, OCTI, OB> &rhs) noexcept;

        template<class OCTR, class OCTI, bool OB>
        self_type &operator*=(const complex<OCTR, OCTI, OB> &rhs) noexcept;

        template<class OCTR, class OCTI, bool OB>
        self_type &operator/=(const complex<OCTR, OCTI, OB> &rhs) noexcept;

        template<class T>
        disable_complex<T, self_type &> operator+=(const T &rhs) noexcept;

        template<class T>
        disable_complex<T, self_type &> operator-=(const T &rhs) noexcept;

        template<class T>
        disable_complex<T, self_type &> operator*=(const T &rhs) noexcept;

        template<class T>
        disable_complex<T, self_type &> operator/=(const T &rhs) noexcept;

        real_reference real() & noexcept;

        real_rvalue_reference real() && noexcept;

        constexpr real_const_reference real() const & noexcept;

        constexpr real_rvalue_const_reference real() const && noexcept;

        imag_reference imag() & noexcept;

        imag_rvalue_reference imag() && noexcept;

        constexpr imag_const_reference imag() const & noexcept;

        constexpr imag_rvalue_const_reference imag() const && noexcept;

        closure_pointer<self_type &> operator&() & noexcept;

        closure_pointer<const self_type &> operator&() const & noexcept;

        closure_pointer<self_type> operator&() && noexcept;

    private:

        CTR m_real;
        CTI m_imag;
    };

    /**********************
     * complex operators *
     **********************/

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    bool operator==(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept;

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    bool operator!=(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept;

    template<class OC, class OT, class CTR, class CTI, bool B>
    std::basic_ostream<OC, OT> &operator<<(std::basic_ostream<OC, OT> &out, const complex<CTR, CTI, B> &c) noexcept;

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    operator+(const complex<CTR, CTI, B> &rhs) noexcept;

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    operator-(const complex<CTR, CTI, B> &rhs) noexcept;

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    operator+(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept;

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator+(const complex<CTR, CTI, B> &lhs, const T &rhs) noexcept;

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator+(const T &lhs, const complex<CTR, CTI, B> &rhs) noexcept;

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    operator-(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept;

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator-(const complex<CTR, CTI, B> &lhs, const T &rhs) noexcept;

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator-(const T &lhs, const complex<CTR, CTI, B> &rhs) noexcept;

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    operator*(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept;

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator*(const complex<CTR, CTI, B> &lhs, const T &rhs) noexcept;

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator*(const T &lhs, const complex<CTR, CTI, B> &rhs) noexcept;

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    operator/(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept;

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator/(const complex<CTR, CTI, B> &lhs, const T &rhs) noexcept;

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator/(const T &lhs, const complex<CTR, CTI, B> &rhs) noexcept;

    /*****************
     * real and imag *
     *****************/

    template<class E>
    decltype(auto) real(E &&e) noexcept;

    template<class E>
    decltype(auto) imag(E &&e) noexcept;

    /***************************
     * complex free functions *
     ***************************/

    template<class CTR, class CTI, bool B>
    typename complex<CTR, CTI, B>::value_type
    abs(const complex<CTR, CTI, B> &rhs);

    template<class CTR, class CTI, bool B>
    typename complex<CTR, CTI, B>::value_type
    arg(const complex<CTR, CTI, B> &rhs);

    template<class CTR, class CTI, bool B>
    typename complex<CTR, CTI, B>::value_type
    norm(const complex<CTR, CTI, B> &rhs);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    conj(const complex<CTR, CTI, B> &rhs);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    proj(const complex<CTR, CTI, B> &rhs);

    /**********************************
     * complex exponential functions *
     **********************************/

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    exp(const complex<CTR, CTI, B> &rhs);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    log(const complex<CTR, CTI, B> &rhs);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    log10(const complex<CTR, CTI, B> &rhs);

    /****************************
     * complex power functions *
     ****************************/

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    pow(const complex<CTR1, CTI1, B1> &x, const complex<CTR2, CTI2, B2> &y);

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    pow(const complex<CTR, CTI, B> &x, const T &y);

    template<class CTR, class CTI, bool B, class T>
    enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    pow(const T &x, const complex<CTR, CTI, B> &y);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    sqrt(const complex<CTR, CTI, B> &x);

    /************************************
     * complex trigonometric functions *
     ************************************/

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    sin(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    cos(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    tan(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    asin(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    acos(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    atan(const complex<CTR, CTI, B> &x);

    /*********************************
     * complex hyperbolic functions *
     *********************************/

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    sinh(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    cosh(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    tanh(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    asinh(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    acosh(const complex<CTR, CTI, B> &x);

    template<class CTR, class CTI, bool B>
    temporary_complex_t<CTR, CTI, B>
    atanh(const complex<CTR, CTI, B> &x);

    /***************************
     * complex implementation *
     ***************************/

    template<class CTR, class CTI, bool B>
    template<class OCTR>
    inline auto complex<CTR, CTI, B>::operator=(OCTR &&rhs) noexcept -> disable_complex<OCTR, self_type &> {
        m_real = std::forward<OCTR>(rhs);
        m_imag = std::decay_t<CTI>();
        return *this;
    }

    template<class CTR, class CTI, bool B>
    template<class OCTR, class OCTI, bool OB>
    inline auto complex<CTR, CTI, B>::operator=(const complex<OCTR, OCTI, OB> &rhs) noexcept -> self_type & {
        m_real = rhs.m_real;
        m_imag = rhs.m_imag;
        return *this;
    }

    template<class CTR, class CTI, bool B>
    template<class OCTR, class OCTI, bool OB>
    inline auto complex<CTR, CTI, B>::operator=(complex<OCTR, OCTI, OB> &&rhs) noexcept -> self_type & {
        m_real = std::move(rhs.m_real);
        m_imag = std::move(rhs.m_imag);
        return *this;
    }

    template<class CTR, class CTI, bool B>
    inline complex<CTR, CTI, B>::operator std::complex<std::decay_t<CTR>>() const noexcept {
        return std::complex<std::decay_t<CTR>>(m_real, m_imag);
    }

    template<class CTR, class CTI, bool B>
    template<class OCTR, class OCTI, bool OB>
    inline auto complex<CTR, CTI, B>::operator+=(const complex<OCTR, OCTI, OB> &rhs) noexcept -> self_type & {
        m_real += rhs.m_real;
        m_imag += rhs.m_imag;
        return *this;
    }

    template<class CTR, class CTI, bool B>
    template<class OCTR, class OCTI, bool OB>
    inline auto complex<CTR, CTI, B>::operator-=(const complex<OCTR, OCTI, OB> &rhs) noexcept -> self_type & {
        m_real -= rhs.m_real;
        m_imag -= rhs.m_imag;
        return *this;
    }

    namespace detail {
        template<bool ieee_compliant>
        struct complex_multiplier {
            template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
            static auto mul(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) {
                using return_type = temporary_complex_t<CTR1, CTI1, B1>;
                using value_type = typename return_type::value_type;
                value_type a = lhs.real();
                value_type b = lhs.imag();
                value_type c = rhs.real();
                value_type d = rhs.imag();
                return return_type(a * c - b * d, a * d + b * c);
            }

            template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
            static auto div(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) {
                using return_type = temporary_complex_t<CTR1, CTI1, B1>;
                using value_type = typename return_type::value_type;
                value_type a = lhs.real();
                value_type b = lhs.imag();
                value_type c = rhs.real();
                value_type d = rhs.imag();
                value_type e = c * c + d * d;
                return return_type((c * a + d * b) / e, (c * b - d * a) / e);
            }
        };

        template<>
        struct complex_multiplier<true> {
            template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
            static auto mul(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) {
                using return_type = temporary_complex_t<CTR1, CTI1, B1>;
                using value_type = typename return_type::value_type;
                value_type a = lhs.real();
                value_type b = lhs.imag();
                value_type c = rhs.real();
                value_type d = rhs.imag();
                value_type ac = a * c;
                value_type bd = b * d;
                value_type ad = a * d;
                value_type bc = b * c;
                value_type x = ac - bd;
                value_type y = ad + bc;
                if (std::isnan(x) && std::isnan(y)) {
                    bool recalc = false;
                    if (std::isinf(a) || std::isinf(b)) {
                        a = copysign(std::isinf(a) ? value_type(1) : value_type(0), a);
                        b = copysign(std::isinf(b) ? value_type(1) : value_type(0), b);
                        if (std::isnan(c)) {
                            c = copysign(value_type(0), c);
                        }
                        if (std::isnan(d)) {
                            d = copysign(value_type(0), d);
                        }
                        recalc = true;
                    }
                    if (std::isinf(c) || std::isinf(d)) {
                        c = copysign(std::isinf(c) ? value_type(1) : value_type(0), c);
                        d = copysign(std::isinf(c) ? value_type(1) : value_type(0), d);
                        if (std::isnan(a)) {
                            a = copysign(value_type(0), a);
                        }
                        if (std::isnan(b)) {
                            b = copysign(value_type(0), b);
                        }
                        recalc = true;
                    }
                    if (!recalc && (std::isinf(ac) || std::isinf(bd) || std::isinf(ad) || std::isinf(bc))) {
                        if (std::isnan(a)) {
                            a = copysign(value_type(0), a);
                        }
                        if (std::isnan(b)) {
                            b = copysign(value_type(0), b);
                        }
                        if (std::isnan(c)) {
                            c = copysign(value_type(0), c);
                        }
                        if (std::isnan(d)) {
                            d = copysign(value_type(0), d);
                        }
                        recalc = true;
                    }
                    if (recalc) {
                        x = std::numeric_limits<value_type>::infinity() * (a * c - b * d);
                        y = std::numeric_limits<value_type>::infinity() * (a * d + b * c);
                    }
                }
                return return_type(x, y);
            }

            template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
            static auto div(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) {
                using return_type = temporary_complex_t<CTR1, CTI1, B1>;
                using value_type = typename return_type::value_type;
                value_type a = lhs.real();
                value_type b = lhs.imag();
                value_type c = rhs.real();
                value_type d = rhs.imag();
                value_type logbw = std::logb(std::fmax(std::fabs(c), std::fabs(d)));
                int ilogbw = 0;
                if (std::isfinite(logbw)) {
                    ilogbw = static_cast<int>(logbw);
                    c = std::scalbn(c, -ilogbw);
                    d = std::scalbn(d, -ilogbw);
                }
                value_type denom = c * c + d * d;
                value_type x = std::scalbn((a * c + b * d) / denom, -ilogbw);
                value_type y = std::scalbn((b * c - a * d) / denom, -ilogbw);
                if (std::isnan(x) && std::isnan(y)) {
                    if ((denom == value_type(0)) && (!std::isnan(a) || !std::isnan(b))) {
                        x = copysign(std::numeric_limits<value_type>::infinity(), c) * a;
                        y = copysign(std::numeric_limits<value_type>::infinity(), c) * b;
                    } else if ((std::isinf(a) || std::isinf(b)) && std::isfinite(c) && std::isfinite(d)) {
                        a = copysign(std::isinf(a) ? value_type(1) : value_type(0), a);
                        b = copysign(std::isinf(b) ? value_type(1) : value_type(0), b);
                        x = std::numeric_limits<value_type>::infinity() * (a * c + b * d);
                        y = std::numeric_limits<value_type>::infinity() * (b * c - a * d);
                    } else if (std::isinf(logbw) && logbw > value_type(0) && std::isfinite(a) && std::isfinite(b)) {
                        c = copysign(std::isinf(c) ? value_type(1) : value_type(0), c);
                        d = copysign(std::isinf(d) ? value_type(1) : value_type(0), d);
                        x = value_type(0) * (a * c + b * d);
                        y = value_type(0) * (b * c - a * d);
                    }
                }
                return std::complex<value_type>(x, y);
            }
        };
    }

    template<class CTR, class CTI, bool B>
    template<class OCTR, class OCTI, bool OB>
    inline auto complex<CTR, CTI, B>::operator*=(const complex<OCTR, OCTI, OB> &rhs) noexcept -> self_type & {
        *this = detail::complex_multiplier<B || OB>::mul(*this, rhs);
        return *this;
    }

    template<class CTR, class CTI, bool B>
    template<class OCTR, class OCTI, bool OB>
    inline auto complex<CTR, CTI, B>::operator/=(const complex<OCTR, OCTI, OB> &rhs) noexcept -> self_type & {
        *this = detail::complex_multiplier<B || OB>::div(*this, rhs);
        return *this;
    }

    template<class CTR, class CTI, bool B>
    template<class T>
    inline auto complex<CTR, CTI, B>::operator+=(const T &rhs) noexcept -> disable_complex<T, self_type &> {
        m_real += rhs;
        return *this;
    }

    template<class CTR, class CTI, bool B>
    template<class T>
    inline auto complex<CTR, CTI, B>::operator-=(const T &rhs) noexcept -> disable_complex<T, self_type &> {
        m_real -= rhs;
        return *this;
    }

    template<class CTR, class CTI, bool B>
    template<class T>
    inline auto complex<CTR, CTI, B>::operator*=(const T &rhs) noexcept -> disable_complex<T, self_type &> {
        m_real *= rhs;
        m_imag *= rhs;
        return *this;
    }

    template<class CTR, class CTI, bool B>
    template<class T>
    inline auto complex<CTR, CTI, B>::operator/=(const T &rhs) noexcept -> disable_complex<T, self_type &> {
        m_real /= rhs;
        m_imag /= rhs;
        return *this;
    }

    template<class CTR, class CTI, bool B>
    auto complex<CTR, CTI, B>::real() & noexcept -> real_reference {
        return m_real;
    }

    template<class CTR, class CTI, bool B>
    auto complex<CTR, CTI, B>::real() && noexcept -> real_rvalue_reference {
        return m_real;
    }

    template<class CTR, class CTI, bool B>
    constexpr auto complex<CTR, CTI, B>::real() const & noexcept -> real_const_reference {
        return m_real;
    }

    template<class CTR, class CTI, bool B>
    constexpr auto complex<CTR, CTI, B>::real() const && noexcept -> real_rvalue_const_reference {
        return m_real;
    }

    template<class CTR, class CTI, bool B>
    auto complex<CTR, CTI, B>::imag() & noexcept -> imag_reference {
        return m_imag;
    }

    template<class CTR, class CTI, bool B>
    auto complex<CTR, CTI, B>::imag() && noexcept -> imag_rvalue_reference {
        return m_imag;
    }

    template<class CTR, class CTI, bool B>
    constexpr auto complex<CTR, CTI, B>::imag() const & noexcept -> imag_const_reference {
        return m_imag;
    }

    template<class CTR, class CTI, bool B>
    constexpr auto complex<CTR, CTI, B>::imag() const && noexcept -> imag_rvalue_const_reference {
        return m_imag;
    }

    template<class CTR, class CTI, bool B>
    inline auto complex<CTR, CTI, B>::operator&() & noexcept -> closure_pointer<self_type &> {
        return closure_pointer<self_type &>(*this);
    }

    template<class CTR, class CTI, bool B>
    inline auto complex<CTR, CTI, B>::operator&() const & noexcept -> closure_pointer<const self_type &> {
        return closure_pointer<const self_type &>(*this);
    }

    template<class CTR, class CTI, bool B>
    inline auto complex<CTR, CTI, B>::operator&() && noexcept -> closure_pointer<self_type> {
        return closure_pointer<self_type>(std::move(*this));
    }

    /*************************************
     * complex operators implementation *
     *************************************/

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    inline bool operator==(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept {
        return lhs.real() == rhs.real() && lhs.imag() == rhs.imag();
    }

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    inline bool operator!=(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept {
        return !(lhs == rhs);
    }

    template<class OC, class OT, class CTR, class CTI, bool B>
    inline std::basic_ostream<OC, OT> &
    operator<<(std::basic_ostream<OC, OT> &out, const complex<CTR, CTI, B> &c) noexcept {
        out << "(" << c.real() << "," << c.imag() << ")";
        return out;
    }

#ifdef __CLING__
    template <class CTR, class CTI, bool B>
    nlohmann::json mime_bundle_repr(const complex<CTR, CTI, B>& c)
    {
        auto bundle = nlohmann::json::object();
        std::stringstream tmp;
        tmp << c;
        bundle["text/plain"] = tmp.str();
        return bundle;
    }
#endif

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    operator+(const complex<CTR, CTI, B> &rhs) noexcept {
        return rhs;
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    operator-(const complex<CTR, CTI, B> &rhs) noexcept {
        return temporary_complex_t<CTR, CTI, B>(-rhs.real(), -rhs.imag());
    }

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    inline common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    operator+(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept {
        common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2> res(lhs);
        res += rhs;
        return res;
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator+(const complex<CTR, CTI, B> &lhs, const T &rhs) noexcept {
        temporary_complex_t<CTR, CTI, B> res(lhs);
        res += rhs;
        return res;
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator+(const T &lhs, const complex<CTR, CTI, B> &rhs) noexcept {
        temporary_complex_t<CTR, CTI, B> res(lhs);
        res += rhs;
        return res;
    }

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    inline common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    operator-(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept {
        common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2> res(lhs);
        res -= rhs;
        return res;
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator-(const complex<CTR, CTI, B> &lhs, const T &rhs) noexcept {
        temporary_complex_t<CTR, CTI, B> res(lhs);
        res -= rhs;
        return res;
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator-(const T &lhs, const complex<CTR, CTI, B> &rhs) noexcept {
        temporary_complex_t<CTR, CTI, B> res(lhs);
        res -= rhs;
        return res;
    }

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    inline common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    operator*(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept {
        common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2> res(lhs);
        res *= rhs;
        return res;
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator*(const complex<CTR, CTI, B> &lhs, const T &rhs) noexcept {
        temporary_complex_t<CTR, CTI, B> res(lhs);
        res *= rhs;
        return res;
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator*(const T &lhs, const complex<CTR, CTI, B> &rhs) noexcept {
        temporary_complex_t<CTR, CTI, B> res(lhs);
        res *= rhs;
        return res;
    }

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    inline common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    operator/(const complex<CTR1, CTI1, B1> &lhs, const complex<CTR2, CTI2, B2> &rhs) noexcept {
        common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2> res(lhs);
        res /= rhs;
        return res;
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator/(const complex<CTR, CTI, B> &lhs, const T &rhs) noexcept {
        temporary_complex_t<CTR, CTI, B> res(lhs);
        res /= rhs;
        return res;
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    operator/(const T &lhs, const complex<CTR, CTI, B> &rhs) noexcept {
        temporary_complex_t<CTR, CTI, B> res(lhs);
        res /= rhs;
        return res;
    }

    /***************************
     * complex free functions *
     ***************************/

    template<class CTR, class CTI, bool B>
    inline typename complex<CTR, CTI, B>::value_type
    abs(const complex<CTR, CTI, B> &rhs) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::abs(std::complex<value_type>(rhs));
    }

    template<class CTR, class CTI, bool B>
    inline typename complex<CTR, CTI, B>::value_type
    arg(const complex<CTR, CTI, B> &rhs) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::arg(std::complex<value_type>(rhs));
    }

    template<class CTR, class CTI, bool B>
    inline typename complex<CTR, CTI, B>::value_type
    norm(const complex<CTR, CTI, B> &rhs) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::norm(std::complex<value_type>(rhs));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    conj(const complex<CTR, CTI, B> &rhs) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::conj(std::complex<value_type>(rhs));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    proj(const complex<CTR, CTI, B> &rhs) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::proj(std::complex<value_type>(rhs));
    }

    /*************************************************
     * complex exponential functions implementation *
     *************************************************/

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    exp(const complex<CTR, CTI, B> &rhs) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::exp(std::complex<value_type>(rhs));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    log(const complex<CTR, CTI, B> &rhs) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::log(std::complex<value_type>(rhs));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    log10(const complex<CTR, CTI, B> &rhs) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::log10(std::complex<value_type>(rhs));
    }

    /*******************************************
     * complex power functions implementation *
     *******************************************/

    template<class CTR1, class CTI1, bool B1, class CTR2, class CTI2, bool B2>
    inline common_complex_t<CTR1, CTI1, B1, CTR2, CTI2, B2>
    pow(const complex<CTR1, CTI1, B1> &x, const complex<CTR2, CTI2, B2> &y) {
        using value_type1 = typename complex<CTR1, CTI1, B1>::value_type;
        using value_type2 = typename complex<CTR2, CTI2, B2>::value_type;
        return std::pow(std::complex<value_type1>(x), std::complex<value_type2>(y));
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    pow(const complex<CTR, CTI, B> &x, const T &y) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::pow(std::complex<value_type>(x), y);
    }

    template<class CTR, class CTI, bool B, class T>
    inline enable_scalar<T, temporary_complex_t<CTR, CTI, B>>
    pow(const T &x, const complex<CTR, CTI, B> &y) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::pow(x, std::complex<value_type>(y));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    sqrt(const complex<CTR, CTI, B> &x) {
        using value_type = typename complex<CTR, CTI, B>::value_type;
        return std::sqrt(std::complex<value_type>(x));
    }

    /***************************************************
     * complex trigonometric functions implementation *
     ***************************************************/

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    sin(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::sin(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    cos(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::cos(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    tan(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::tan(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    asin(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::asin(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    acos(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::acos(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    atan(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::atan(std::complex<value_type>(x));
    }

    /************************************************
     * complex hyperbolic functions implementation *
     ************************************************/

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    sinh(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::sinh(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    cosh(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::cosh(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    tanh(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::tanh(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    asinh(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::asinh(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    acosh(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::acosh(std::complex<value_type>(x));
    }

    template<class CTR, class CTI, bool B>
    inline temporary_complex_t<CTR, CTI, B>
    atanh(const complex<CTR, CTI, B> &x) {
        using value_type = typename temporary_complex_t<CTR, CTI, B>::value_type;
        return std::atanh(std::complex<value_type>(x));
    }

    /*********************************
     * forward_offset implementation *
     *********************************/

    namespace detail {

        template<class T, class M>
        struct forward_type {
            using type = apply_cv_t<T, M>;
        };

        template<class T, class M>
        struct forward_type<T &, M> {
            using type = apply_cv_t<T, M> &;
        };

        template<class T, class M>
        using forward_type_t = typename forward_type<T, M>::type;
    }

    template<class M, std::size_t I, class T>
    constexpr detail::forward_type_t<T, M> forward_offset(T &&v) noexcept {
        using forward_type = detail::forward_type_t<T, M>;
        using cv_value_type = std::remove_reference_t<forward_type>;
        using byte_type = apply_cv_t<std::remove_reference_t<T>, char>;

        return static_cast<forward_type>(
                *reinterpret_cast<cv_value_type *>(
                        reinterpret_cast<byte_type *>(&v) + I
                )
        );
    }

    /**********************************************
     * forward_real & forward_imag implementation *
     **********************************************/

    // forward_real

    template<class T>
    auto forward_real(T &&v)
    -> std::enable_if_t<!is_gen_complex<T>::value, detail::forward_type_t<T, T>>  // real case -> forward
    {
        return static_cast<detail::forward_type_t<T, T>>(v);
    }

    template<class T>
    auto forward_real(T &&v)
    -> std::enable_if_t<is_std_complex<T>::value, detail::forward_type_t<T, typename std::decay_t<T>::value_type>>  // complex case -> forward the real part
    {
        return forward_offset<typename std::decay_t<T>::value_type, 0>(v);
    }

    template<class T>
    auto forward_real(T &&v)
    -> std::enable_if_t<is_complex<T>::value, decltype(std::forward<T>(v).real())> {
        return std::forward<T>(v).real();
    }

    // forward_imag

    template<class T>
    auto forward_imag(T &&)
    -> std::enable_if_t<!is_gen_complex<T>::value, std::decay_t<T>>  // real case -> always return 0 by value
    {
        return 0;
    }

    template<class T>
    auto forward_imag(T &&v)
    -> std::enable_if_t<is_std_complex<T>::value, detail::forward_type_t<T, typename std::decay_t<T>::value_type>>  // complex case -> forwards the imaginary part
    {
        using real_type = typename std::decay_t<T>::value_type;
        return forward_offset<real_type, sizeof(real_type)>(v);
    }

    template<class T>
    auto forward_imag(T &&v)
    -> std::enable_if_t<is_complex<T>::value, decltype(std::forward<T>(v).imag())> {
        return std::forward<T>(v).imag();
    }

    /******************************
     * real & imag implementation *
     ******************************/

    template<class E>
    inline decltype(auto) real(E &&e) noexcept {
        return forward_real(std::forward<E>(e));
    }

    template<class E>
    inline decltype(auto) imag(E &&e) noexcept {
        return forward_imag(std::forward<E>(e));
    }

    /**********************
     * complex_value_type *
     **********************/

    template<class T>
    struct complex_value_type {
        using type = T;
    };

    template<class T>
    struct complex_value_type<std::complex<T>> {
        using type = T;
    };

    template<class CTR, class CTI, bool B>
    struct complex_value_type<complex<CTR, CTI, B>> {
        using type = complex<CTR, CTI, B>;
    };

    template<class T>
    using complex_value_type_t = typename complex_value_type<T>::type;

    /******************************************************
     * operator overloads for complex and closure wrapper *
     *****************************************************/

    template<class C, class T, std::enable_if_t<!is_std_complex<T>::value, int> = 0>
    std::complex<C> operator+(const std::complex<C> &c, const T &t) {
        std::complex<C> result(c);
        result += t;
        return result;
    }

    template<class C, class T, std::enable_if_t<!is_std_complex<T>::value, int> = 0>
    std::complex<C> operator+(const T &t, const std::complex<C> &c) {
        std::complex<C> result(t);
        result += c;
        return result;
    }

    template<class C, class T, std::enable_if_t<!is_std_complex<T>::value, int> = 0>
    std::complex<C> operator-(const std::complex<C> &c, const T &t) {
        std::complex<C> result(c);
        result -= t;
        return result;
    }

    template<class C, class T, std::enable_if_t<!is_std_complex<T>::value, int> = 0>
    std::complex<C> operator-(const T &t, const std::complex<C> &c) {
        std::complex<C> result(t);
        result -= c;
        return result;
    }

    template<class C, class T, std::enable_if_t<!is_std_complex<T>::value, int> = 0>
    std::complex<C> operator*(const std::complex<C> &c, const T &t) {
        std::complex<C> result(c);
        result *= t;
        return result;
    }

    template<class C, class T, std::enable_if_t<!is_std_complex<T>::value, int> = 0>
    std::complex<C> operator*(const T &t, const std::complex<C> &c) {
        std::complex<C> result(t);
        result *= c;
        return result;
    }

    template<class C, class T, std::enable_if_t<!is_std_complex<T>::value, int> = 0>
    std::complex<C> operator/(const std::complex<C> &c, const T &t) {
        std::complex<C> result(c);
        result /= t;
        return result;
    }

    template<class C, class T, std::enable_if_t<!is_std_complex<T>::value, int> = 0>
    std::complex<C> operator/(const T &t, const std::complex<C> &c) {
        std::complex<C> result(t);
        result /= c;
        return result;
    }
}

#endif  // TURBO_META_COMPLEX_H_

