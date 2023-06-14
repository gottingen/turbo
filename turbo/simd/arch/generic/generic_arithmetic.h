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


#ifndef TURBO_SIMD_ARCH_GENERIC_ARITHMETIC_H_
#define TURBO_SIMD_ARCH_GENERIC_ARITHMETIC_H_

#include <complex>
#include <type_traits>

#include "turbo/simd/arch/generic/generic_details.h"

namespace turbo::simd {

    namespace kernel {

        using namespace types;

        // bitwise_lshift
        template<class A, class T, class /*=typename std::enable_if<std::is_integral<T>::value, void>::type*/>
        inline batch<T, A>
        bitwise_lshift(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
            return detail::apply([](T x, T y) noexcept { return x << y; },
                                 self, other);
        }

        // bitwise_rshift
        template<class A, class T, class /*=typename std::enable_if<std::is_integral<T>::value, void>::type*/>
        inline batch<T, A>
        bitwise_rshift(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
            return detail::apply([](T x, T y) noexcept { return x >> y; },
                                 self, other);
        }

        // decr
        template<class A, class T>
        inline batch<T, A> decr(batch<T, A> const &self, requires_arch<generic>) noexcept {
            return self - T(1);
        }

        // decr_if
        template<class A, class T, class Mask>
        inline batch<T, A> decr_if(batch<T, A> const &self, Mask const &mask, requires_arch<generic>) noexcept {
            return select(mask, decr(self), self);
        }

        // div
        template<class A, class T, class = typename std::enable_if<std::is_integral<T>::value, void>::type>
        inline batch<T, A> div(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
            return detail::apply([](T x, T y) noexcept -> T { return x / y; },
                                 self, other);
        }

        // fma
        template<class A, class T>
        inline batch<T, A>
        fma(batch<T, A> const &x, batch<T, A> const &y, batch<T, A> const &z, requires_arch<generic>) noexcept {
            return x * y + z;
        }

        template<class A, class T>
        inline batch<std::complex<T>, A>
        fma(batch<std::complex<T>, A> const &x, batch<std::complex<T>, A> const &y, batch<std::complex<T>, A> const &z,
            requires_arch<generic>) noexcept {
            auto res_r = fms(x.real(), y.real(), fms(x.imag(), y.imag(), z.real()));
            auto res_i = fma(x.real(), y.imag(), fma(x.imag(), y.real(), z.imag()));
            return {res_r, res_i};
        }

        // fms
        template<class A, class T>
        inline batch<T, A>
        fms(batch<T, A> const &x, batch<T, A> const &y, batch<T, A> const &z, requires_arch<generic>) noexcept {
            return x * y - z;
        }

        template<class A, class T>
        inline batch<std::complex<T>, A>
        fms(batch<std::complex<T>, A> const &x, batch<std::complex<T>, A> const &y, batch<std::complex<T>, A> const &z,
            requires_arch<generic>) noexcept {
            auto res_r = fms(x.real(), y.real(), fma(x.imag(), y.imag(), z.real()));
            auto res_i = fma(x.real(), y.imag(), fms(x.imag(), y.real(), z.imag()));
            return {res_r, res_i};
        }

        // fnma
        template<class A, class T>
        inline batch<T, A>
        fnma(batch<T, A> const &x, batch<T, A> const &y, batch<T, A> const &z, requires_arch<generic>) noexcept {
            return -x * y + z;
        }

        template<class A, class T>
        inline batch<std::complex<T>, A>
        fnma(batch<std::complex<T>, A> const &x, batch<std::complex<T>, A> const &y, batch<std::complex<T>, A> const &z,
             requires_arch<generic>) noexcept {
            auto res_r = -fms(x.real(), y.real(), fma(x.imag(), y.imag(), z.real()));
            auto res_i = -fma(x.real(), y.imag(), fms(x.imag(), y.real(), z.imag()));
            return {res_r, res_i};
        }

        // fnms
        template<class A, class T>
        inline batch<T, A>
        fnms(batch<T, A> const &x, batch<T, A> const &y, batch<T, A> const &z, requires_arch<generic>) noexcept {
            return -x * y - z;
        }

        template<class A, class T>
        inline batch<std::complex<T>, A>
        fnms(batch<std::complex<T>, A> const &x, batch<std::complex<T>, A> const &y, batch<std::complex<T>, A> const &z,
             requires_arch<generic>) noexcept {
            auto res_r = -fms(x.real(), y.real(), fms(x.imag(), y.imag(), z.real()));
            auto res_i = -fma(x.real(), y.imag(), fma(x.imag(), y.real(), z.imag()));
            return {res_r, res_i};
        }

        // incr
        template<class A, class T>
        inline batch<T, A> incr(batch<T, A> const &self, requires_arch<generic>) noexcept {
            return self + T(1);
        }

        // incr_if
        template<class A, class T, class Mask>
        inline batch<T, A> incr_if(batch<T, A> const &self, Mask const &mask, requires_arch<generic>) noexcept {
            return select(mask, incr(self), self);
        }

        // mul
        template<class A, class T, class /*=typename std::enable_if<std::is_integral<T>::value, void>::type*/>
        inline batch<T, A> mul(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
            return detail::apply([](T x, T y) noexcept -> T { return x * y; },
                                 self, other);
        }

        // sadd
        template<class A>
        inline batch<float, A>
        sadd(batch<float, A> const &self, batch<float, A> const &other, requires_arch<generic>) noexcept {
            return add(self, other); // no saturated arithmetic on floating point numbers
        }

        template<class A>
        inline batch<double, A>
        sadd(batch<double, A> const &self, batch<double, A> const &other, requires_arch<generic>) noexcept {
            return add(self, other); // no saturated arithmetic on floating point numbers
        }

        // ssub
        template<class A>
        inline batch<float, A>
        ssub(batch<float, A> const &self, batch<float, A> const &other, requires_arch<generic>) noexcept {
            return sub(self, other); // no saturated arithmetic on floating point numbers
        }

        template<class A>
        inline batch<double, A>
        ssub(batch<double, A> const &self, batch<double, A> const &other, requires_arch<generic>) noexcept {
            return sub(self, other); // no saturated arithmetic on floating point numbers
        }

    }

}

#endif  // TURBO_SIMD_ARCH_GENERIC_ARITHMETIC_H_

