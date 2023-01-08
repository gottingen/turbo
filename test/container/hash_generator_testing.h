
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef HASH_GENERATOR_TESTING_H_
#define HASH_GENERATOR_TESTING_H_

#include <stdint.h>
#include <algorithm>
#include <iosfwd>
#include <random>
#include <tuple>
#include <type_traits>
#include <utility>
#include <string>
#include <deque>
#include <functional>
#include <string_view>

#include "hash_policy_testing.h"

namespace flare {
    namespace priv {
        namespace hash_internal {
            namespace generator_internal {

                template<class Container, class = void>
                struct IsMap : std::false_type {
                };

                template<class Map>
                struct IsMap<Map, flare::void_t<typename Map::mapped_type>> : std::true_type {
                };

            }  // namespace generator_internal

            namespace {
                class RandomDeviceSeedSeq {
                public:
                    using result_type = typename std::random_device::result_type;

                    template<class Iterator>
                    void generate(Iterator start, Iterator end) {
                        while (start != end) {
                            *start = gen_();
                            ++start;
                        }
                    }

                private:
                    std::random_device gen_;
                };
            }  // namespace

            std::mt19937_64 *GetSharedRng(); // declaration

            std::mt19937_64 *GetSharedRng() {
                RandomDeviceSeedSeq seed_seq;
                static auto *rng = new std::mt19937_64(seed_seq);
                return rng;
            }


            enum Enum {
                kEnumEmpty,
                kEnumDeleted,
            };

            enum class EnumClass : uint64_t {
                kEmpty,
                kDeleted,
            };

            inline std::ostream &operator<<(std::ostream &o, const EnumClass &ec) {
                return o << static_cast<uint64_t>(ec);
            }

            template<class T, class E = void>
            struct Generator;

            template<class T>
            struct Generator<T, typename std::enable_if<std::is_integral<T>::value>::type> {
                T operator()() const {
                    std::uniform_int_distribution<T> dist;
                    return dist(*GetSharedRng());
                }
            };

            template<>
            struct Generator<Enum> {
                Enum operator()() const {
                    std::uniform_int_distribution<typename std::underlying_type<Enum>::type> dist;

                    while (true) {
                        auto variate = dist(*GetSharedRng());
                        if (variate != kEnumEmpty && variate != kEnumDeleted)
                            return static_cast<Enum>(variate);
                    }
                }
            };

            template<>
            struct Generator<EnumClass> {
                EnumClass operator()() const {
                    std::uniform_int_distribution<
                            typename std::underlying_type<EnumClass>::type> dist;
                    while (true) {
                        EnumClass variate = static_cast<EnumClass>(dist(*GetSharedRng()));
                        if (variate != EnumClass::kEmpty && variate != EnumClass::kDeleted)
                            return static_cast<EnumClass>(variate);
                    }
                }
            };

            template<>
            struct Generator<std::string> {
                std::string operator()() const {
                    // NOLINTNEXTLINE(runtime/int)
                    std::uniform_int_distribution<short> chars(0x20, 0x7E);
                    std::string res;
                    res.resize(32);
                    std::generate(res.begin(), res.end(),
                                  [&]() { return (char) chars(*GetSharedRng()); });
                    return res;
                }
            };


            template<>
            struct Generator<std::string_view> {
                std::string_view operator()() const {
                    static auto *arena = new std::deque<std::string>();
                    // NOLINTNEXTLINE(runtime/int)
                    std::uniform_int_distribution<short> chars(0x20, 0x7E);
                    arena->emplace_back();
                    auto &res = arena->back();
                    res.resize(32);
                    std::generate(res.begin(), res.end(),
                                  [&]() { return (char) chars(*GetSharedRng()); });
                    return res;
                }
            };


            template<>
            struct Generator<NonStandardLayout> {
                NonStandardLayout operator()() const {
                    return NonStandardLayout(Generator<std::string>()());
                }
            };

            template<class K, class V>
            struct Generator<std::pair<K, V>> {
                std::pair<K, V> operator()() const {
                    return std::pair<K, V>(Generator<typename std::decay<K>::type>()(),
                                           Generator<typename std::decay<V>::type>()());
                }
            };

            template<class... Ts>
            struct Generator<std::tuple<Ts...>> {
                std::tuple<Ts...> operator()() const {
                    return std::tuple<Ts...>(Generator<typename std::decay<Ts>::type>()()...);
                }
            };

            template<class U>
            struct Generator<U, flare::void_t<decltype(std::declval<U &>().key()),
                    decltype(std::declval<U &>().value())>>
                    : Generator<std::pair<
                            typename std::decay<decltype(std::declval<U &>().key())>::type,
                            typename std::decay<decltype(std::declval<U &>().value())>::type>> {
            };

            template<class Container>
            using GeneratedType = decltype(
            std::declval<const Generator<
                    typename std::conditional<generator_internal::IsMap<Container>::value,
                            typename Container::value_type,
                            typename Container::key_type>::type> &>()());

        }  // namespace hash_internal
    }  // namespace priv
}  // namespace flare

namespace std {
    using flare::priv::hash_internal::EnumClass;
    using flare::priv::hash_internal::Enum;

    template<>
    struct hash<EnumClass> {
        std::size_t operator()(EnumClass const &p) const { return (std::size_t) p; }
    };

    template<>
    struct hash<Enum> {
        std::size_t operator()(Enum const &p) const { return (std::size_t) p; }
    };
}


#endif  // HASH_GENERATOR_TESTING_H_
