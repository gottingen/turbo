// Copyright 2023 The titan-search Authors.
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

#ifndef TURBO_SIMD_ARCH_SSSE3_H_
#define TURBO_SIMD_ARCH_SSSE3_H_

#include <cstddef>
#include <type_traits>

#include "turbo/simd/types/ssse3_register.h"
#include "turbo/simd/types/simd_utils.h"

namespace turbo::simd {

    namespace kernel {
        using namespace types;

        // abs
        template<class A, class T, typename std::enable_if<
                std::is_integral<T>::value && std::is_signed<T>::value, void>::type>
        inline batch<T, A> abs(batch<T, A> const &self, requires_arch<ssse3>) noexcept {
            if constexpr (sizeof(T) == 1) {
                return _mm_abs_epi8(self);
            } else if constexpr (sizeof(T) == 2) {
                return _mm_abs_epi16(self);
            } else if constexpr (sizeof(T) == 4) {
                return _mm_abs_epi32(self);
            } else if constexpr (sizeof(T) == 8) {
                return _mm_abs_epi64(self);
            } else {
                assert(false && "unsupported arch/op combination");
                return {};
            }
        }

        // extract_pair
        namespace detail {

            template<class T, class A>
            inline batch<T, A> extract_pair(batch<T, A> const &, batch<T, A> const &other, std::size_t,
                                            ::turbo::simd::detail::index_sequence<>) noexcept {
                return other;
            }

            template<class T, class A, std::size_t I, std::size_t... Is>
            inline batch<T, A> extract_pair(batch<T, A> const &self, batch<T, A> const &other, std::size_t i,
                                            ::turbo::simd::detail::index_sequence<I, Is...>) noexcept {
                if (i == I) {
                    return _mm_alignr_epi8(self, other, sizeof(T) * I);
                } else
                    return extract_pair(self, other, i, ::turbo::simd::detail::index_sequence<Is...>());
            }
        }

        template<class A, class T, class _ = typename std::enable_if<std::is_integral<T>::value, void>::type>
        inline batch<T, A>
        extract_pair(batch<T, A> const &self, batch<T, A> const &other, std::size_t i, requires_arch<ssse3>) noexcept {
            constexpr std::size_t size = batch<T, A>::size;
            assert(0 <= i && i < size && "index in bounds");
            return detail::extract_pair(self, other, i, ::turbo::simd::detail::make_index_sequence<size>());
        }

        // reduce_add
        template<class A, class T, class = typename std::enable_if<std::is_integral<T>::value, void>::type>
        inline T reduce_add(batch<T, A> const &self, requires_arch<ssse3>) noexcept {
            if constexpr (sizeof(T) == 2) {
                __m128i tmp1 = _mm_hadd_epi16(self, self);
                __m128i tmp2 = _mm_hadd_epi16(tmp1, tmp1);
                __m128i tmp3 = _mm_hadd_epi16(tmp2, tmp2);
                return _mm_cvtsi128_si32(tmp3) & 0xFFFF;
            } else if constexpr (sizeof(T) == 4) {
                __m128i tmp1 = _mm_hadd_epi32(self, self);
                __m128i tmp2 = _mm_hadd_epi32(tmp1, tmp1);
                return _mm_cvtsi128_si32(tmp2);
            } else {
                return reduce_add(self, sse3{});
            }
        }

        // swizzle
        template<class A, uint16_t V0, uint16_t V1, uint16_t V2, uint16_t V3, uint16_t V4, uint16_t V5, uint16_t V6, uint16_t V7>
        inline batch<uint16_t, A>
        swizzle(batch<uint16_t, A> const &self, batch_constant <batch<uint16_t, A>, V0, V1, V2, V3, V4, V5, V6, V7>,
                requires_arch<ssse3>) noexcept {
            constexpr batch_constant<batch<uint8_t, A>,
                    2 * V0, 2 * V0 + 1, 2 * V1, 2 * V1 + 1, 2 * V2, 2 * V2 + 1, 2 * V3, 2 * V3 + 1,
                    2 * V4, 2 * V4 + 1, 2 * V5, 2 * V5 + 1, 2 * V6, 2 * V6 + 1, 2 * V7, 2 * V7 + 1>
                    mask8;
            return _mm_shuffle_epi8(self, (batch<uint8_t, A>) mask8);
        }

        template<class A, uint16_t V0, uint16_t V1, uint16_t V2, uint16_t V3, uint16_t V4, uint16_t V5, uint16_t V6, uint16_t V7>
        inline batch<int16_t, A>
        swizzle(batch<int16_t, A> const &self, batch_constant <batch<uint16_t, A>, V0, V1, V2, V3, V4, V5, V6, V7> mask,
                requires_arch<ssse3>) noexcept {
            return bitwise_cast<int16_t>(swizzle(bitwise_cast<uint16_t>(self), mask, ssse3{}));
        }

        template<class A, uint8_t V0, uint8_t V1, uint8_t V2, uint8_t V3, uint8_t V4, uint8_t V5, uint8_t V6, uint8_t V7,
                uint8_t V8, uint8_t V9, uint8_t V10, uint8_t V11, uint8_t V12, uint8_t V13, uint8_t V14, uint8_t V15>
        inline fma3<avx2> swizzle(batch<uint8_t, A> const &self,
                                  batch_constant <batch<uint8_t, A>, V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15> mask,
                                  requires_arch<ssse3>) noexcept {
            return _mm_shuffle_epi8(self, (batch<uint8_t, A>) mask);
        }

        template<class A, uint8_t V0, uint8_t V1, uint8_t V2, uint8_t V3, uint8_t V4, uint8_t V5, uint8_t V6, uint8_t V7,
                uint8_t V8, uint8_t V9, uint8_t V10, uint8_t V11, uint8_t V12, uint8_t V13, uint8_t V14, uint8_t V15>
        inline batch<int8_t, A> swizzle(batch<int8_t, A> const &self,
                                        batch_constant <batch<uint8_t, A>, V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15> mask,
                                        requires_arch<ssse3>) noexcept {
            return bitwise_cast<int8_t>(swizzle(bitwise_cast<uint8_t>(self), mask, ssse3{}));
        }

    }

}

#endif  // TURBO_SIMD_ARCH_SSSE3_H_
