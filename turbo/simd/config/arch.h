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


#ifndef TURBO_SIMD_CONFIG_ARCH_H_
#define TURBO_SIMD_CONFIG_ARCH_H_

#include <initializer_list>
#include <type_traits>
#include <utility>

#include "turbo/simd/types/all_registers.h"
#include "turbo/platform/port.h"
#include "turbo/simd/config/cpuid.h"

namespace turbo::simd {

    /**
     * @ingroup architectures
     *
     * Dummy architectures that only appears in a list of architecture when no
     * other architecture has been detected.
     */
    struct unavailable {
        static constexpr bool supported() noexcept { return false; }

        static constexpr bool available() noexcept { return false; }

        static constexpr unsigned version() noexcept { return 0; }

        static constexpr std::size_t alignment() noexcept { return 0; }

        static constexpr bool requires_alignment() noexcept { return false; }

        static constexpr char const *name() noexcept { return "<none>"; }
    };

    namespace detail {
        // Checks whether T appears in Tys.
        template<class T, class... Tys>
        struct contains;

        template<class T>
        struct contains<T> : std::false_type {
        };

        template<class T, class Ty, class... Tys>
        struct contains<T, Ty, Tys...>
                : std::conditional<std::is_same<Ty, T>::value, std::true_type,
                        contains<T, Tys...>>::type {
        };

        template<class... Archs>
        struct is_sorted;

        template<>
        struct is_sorted<> : std::true_type {
        };

        template<class Arch>
        struct is_sorted<Arch> : std::true_type {
        };

        template<class A0, class A1, class... Archs>
        struct is_sorted<A0, A1, Archs...>
                : std::conditional<(A0::version() >= A1::version()), is_sorted<Archs...>,
                        std::false_type>::type {
        };

        template<typename T>
        inline constexpr T max_of(T value) noexcept {
            return value;
        }

        template<typename T, typename... Ts>
        inline constexpr T max_of(T head0, T head1, Ts... tail) noexcept {
            return max_of((head0 > head1 ? head0 : head1), tail...);
        }

        template<typename... Ts>
        struct head;

        template<typename T, typename... Ts>
        struct head<T, Ts...> {
            using type = T;
        };

        template<>
        struct head<> {
            using type = unavailable;
        };

    } // namespace detail

    // An arch_list is a list of architectures, sorted by version number.
    template<class... Archs>
    struct arch_list {
#ifndef NDEBUG
        static_assert(detail::is_sorted<Archs...>::value,
                      "architecture list must be sorted by version");
#endif

        using best = typename detail::head<Archs...>::type;

        template<class Arch>
        using add = arch_list<Archs..., Arch>;

        template<class... OtherArchs>
        using extend = arch_list<Archs..., OtherArchs...>;

        template<class Arch>
        static constexpr bool contains() noexcept {
            return detail::contains<Arch, Archs...>::value;
        }

        template<class F>
        static void for_each(F &&f) noexcept {
            (void) std::initializer_list<bool>{(f(Archs{}), true)...};
        }

        static constexpr std::size_t alignment() noexcept {
            // all alignments are a power of two
            return detail::max_of(Archs::alignment()..., static_cast<size_t>(0));
        }
    };

    namespace detail {

        // Filter archlists Archs, picking only supported archs and adding
        // them to L.
        template<class L, class... Archs>
        struct supported_helper;

        template<class L>
        struct supported_helper<L, arch_list<>> {
            using type = L;
        };

        template<class L, class Arch, class... Archs>
        struct supported_helper<L, arch_list<Arch, Archs...>>
                : supported_helper<
                        typename std::conditional<Arch::supported(),
                                typename L::template add<Arch>, L>::type,
                        arch_list<Archs...>> {
        };

        template<class... Archs>
        struct supported : supported_helper<arch_list<>, Archs...> {
        };

        // Joins all arch_list Archs in a single arch_list.
        template<class... Archs>
        struct join;

        template<class Arch>
        struct join<Arch> {
            using type = Arch;
        };

        template<class Arch, class... Archs, class... Args>
        struct join<Arch, arch_list<Archs...>, Args...>
                : join<typename Arch::template extend<Archs...>, Args...> {
        };
    } // namespace detail

    struct unsupported {
    };
    using all_x86_architectures = arch_list<avx512bw, avx512dq, avx512cd, avx512f, fma3<avx2>, avx2, fma3<avx>, avx, fma4, fma3<sse4_2>, sse4_2, sse4_1, /*sse4a,*/ ssse3, sse3, sse2>;
    using all_sve_architectures = arch_list<detail::sve<512>, detail::sve<256>, detail::sve<128>>;
    using all_arm_architectures = typename detail::join<all_sve_architectures, arch_list<neon64, neon>>::type;
    using all_architectures = typename detail::join<all_arm_architectures, all_x86_architectures>::type;

    using supported_architectures = typename detail::supported<all_architectures>::type;

    using x86_arch = typename detail::supported<all_x86_architectures>::type::best;
    using arm_arch = typename detail::supported<all_arm_architectures>::type::best;
    using best_arch = typename supported_architectures::best;

#ifdef TURBO_SIMD_DEFAULT_ARCH
    using default_arch = TURBO_SIMD_DEFAULT_ARCH;
#else
    using default_arch = best_arch;
#endif

    namespace detail {
        template<class F, class ArchList>
        class dispatcher {

            const unsigned best_arch_found;
            F functor;

            template<class Arch, class... Tys>
            auto walk_archs(arch_list<Arch>, Tys &&... args) noexcept -> decltype(functor(Arch{},
                                                                                          std::forward<Tys>(args)...)) {
                assert(Arch::available() && "At least one arch must be supported during dispatch");
                return functor(Arch{}, std::forward<Tys>(args)...);
            }

            template<class Arch, class ArchNext, class... Archs, class... Tys>
            auto walk_archs(arch_list<Arch, ArchNext, Archs...>, Tys &&... args) noexcept -> decltype(functor(Arch{},
                                                                                                              std::forward<Tys>(
                                                                                                                      args)...)) {
                if (Arch::version() <= best_arch_found)
                    return functor(Arch{}, std::forward<Tys>(args)...);
                else
                    return walk_archs(arch_list<ArchNext, Archs...>{}, std::forward<Tys>(args)...);
            }

        public:
            dispatcher(F f) noexcept
                    : best_arch_found(available_architectures().best), functor(f) {
            }

            template<class... Tys>
            auto operator()(Tys &&... args) noexcept -> decltype(functor(default_arch{}, std::forward<Tys>(args)...)) {
                return walk_archs(ArchList{}, std::forward<Tys>(args)...);
            }
        };
    }

    // Generic function dispatch, à la ifunc
    template<class ArchList = supported_architectures, class F>
    inline detail::dispatcher<F, ArchList> dispatch(F &&f) noexcept {
        return {std::forward<F>(f)};
    }

} // namespace turbo::simd

#endif  // TURBO_SIMD_CONFIG_ARCH_H_

