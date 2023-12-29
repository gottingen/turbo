// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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


#ifndef TURBO_UNICODE_UNICODE_ENGINE_H_
#define TURBO_UNICODE_UNICODE_ENGINE_H_

#include <initializer_list>
#include <type_traits>
#include <utility>
#include "turbo/platform/port.h"
#include "turbo/unicode/all_engine.h"

namespace turbo::unicode {

    struct unavailable_engine {
        static constexpr bool supported() noexcept { return false; }

        static constexpr bool available() noexcept { return false; }

        static constexpr unsigned version() noexcept { return 0; }

        static constexpr std::size_t alignment() noexcept { return 0; }

        static constexpr bool requires_alignment() noexcept { return false; }

        static constexpr char const *name() noexcept { return "<none>"; }
    };

    namespace utf_internal {
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

        template<class... Engines>
        struct is_sorted;

        template<>
        struct is_sorted<> : std::true_type {
        };

        template<class Engine>
        struct is_sorted<Engine> : std::true_type {
        };

        template<class A0, class A1, class... Engines>
        struct is_sorted<A0, A1, Engines...>
                : std::conditional<(A0::version() >= A1::version()), is_sorted<Engines...>,
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
            using type = unavailable_engine;
        };

    } // namespace utf_internal

    // An engine_list is a list of architectures, sorted by version number.
    template<class... Engines>
    struct engine_list {
#ifndef NDEBUG
        static_assert(utf_internal::is_sorted<Engines...>::value,
                      "engine list must be sorted by version");
#endif

        using best = typename utf_internal::head<Engines...>::type;

        template<class Engine>
        using add = engine_list<Engines..., Engine>;

        template<class... OtherArchs>
        using extend = engine_list<Engines..., OtherArchs...>;

        template<class Engine>
        static constexpr bool contains() noexcept {
            return utf_internal::contains<Engine, Engines...>::value;
        }

        template<class F>
        static void for_each(F &&f) noexcept {
            (void) std::initializer_list<bool>{(f(Engines{}), true)...};
        }

        static constexpr std::size_t alignment() noexcept {
            // all alignments are a power of two
            return utf_internal::max_of(Engines::alignment()..., static_cast<size_t>(0));
        }
    };

    namespace utf_internal {

        // Filter archlists Engines, picking only supported archs and adding
        // them to L.
        template<class L, class... Engines>
        struct supported_helper;

        template<class L>
        struct supported_helper<L, engine_list<>> {
            using type = L;
        };

        template<class L, class Engine, class... Engines>
        struct supported_helper<L, engine_list<Engine, Engines...>>
                : supported_helper<
                        typename std::conditional<Engine::supported(),
                                typename L::template add<Engine>, L>::type,
                        engine_list<Engines...>> {
        };

        template<class... Engines>
        struct supported : supported_helper<engine_list<>, Engines...> {
        };

        // Joins all engine_list Engines in a single engine_list.
        template<class... Engines>
        struct join;

        template<class Engine>
        struct join<Engine> {
            using type = Engine;
        };

        template<class Engine, class... Engines, class... Args>
        struct join<Engine, engine_list<Engines...>, Args...>
                : join<typename Engine::template extend<Engines...>, Args...> {
        };
    } // namespace utf_internal

    struct unsupported {
    };
    using last_engine = engine_list<scalar_engine>;
    using all_x86_engines = engine_list<avx2_engine>;
    using all_sve_engines = engine_list<>;
    using all_arm_engines = typename utf_internal::join<all_sve_engines>::type;
    using all_engines = typename utf_internal::join<all_arm_engines, all_x86_engines, last_engine>::type;

    using supported_engines = typename utf_internal::supported<all_engines>::type;

    using x86_arch = typename utf_internal::supported<all_x86_engines>::type::best;
    using arm_arch = typename utf_internal::supported<all_arm_engines>::type::best;
    using best_engine = typename supported_engines::best;

#ifdef TURBO_UNICODE_DEFAULT_ENGINE
    using default_engine = TURBO_UNICODE_DEFAULT_ENGINE;
#else
    using default_engine = best_engine;
#endif
}  // namespace turbo::unicode

#endif  // TURBO_UNICODE_UNICODE_ENGINE_H_
