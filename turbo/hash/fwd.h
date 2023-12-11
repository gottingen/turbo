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
//
// Created by jeff on 23-12-11.
//

#ifndef TURBO_HASH_FWD_H_
#define TURBO_HASH_FWD_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace turbo {

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

    /// forward declarations

    template<int n>
    struct MixEngine {
        static constexpr size_t mix(size_t key);
    };

    template<typename E>
    struct is_mix_engine : public std::false_type {};

    /**
     * @brief interface for hash engine
     * @tparam Tag
     */
    struct hash_engine_tag {

        constexpr static const char* name();

        constexpr static bool available();
    };

    template <typename Tag>
    struct hasher_engine;

}  // namespace turbo

#endif  // TURBO_HASH_FWD_H_
