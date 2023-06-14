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


#ifndef TURBO_SIMD_TYPES_BATCH_CONSTANT_H_
#define TURBO_SIMD_TYPES_BATCH_CONSTANT_H_

#include "turbo/simd/types/batch.h"
#include "turbo/simd/types/utils.h"

namespace turbo::simd
{
    /**
     * @brief batch of boolean constant
     *
     * Abstract representation of a batch of boolean constants.
     *
     * @tparam batch_type the type of the associated batch values.
     * @tparam Values boolean constant represented by this batch
     **/
    template <class batch_type, bool... Values>
    struct batch_bool_constant
    {
        static constexpr std::size_t size = sizeof...(Values);
        using arch_type = typename batch_type::arch_type;
        using value_type = bool;
        static_assert(sizeof...(Values) == batch_type::size, "consistent batch size");

        operator batch_bool<typename batch_type::value_type, arch_type>() const noexcept { return { Values... }; }

        bool get(size_t i) const noexcept
        {
            return std::array<value_type, size> { { Values... } }[i];
        }

        static constexpr int mask() noexcept
        {
            return mask_helper(0, static_cast<int>(Values)...);
        }

    private:
        static constexpr int mask_helper(int acc) noexcept { return acc; }
        template <class... Tys>
        static constexpr int mask_helper(int acc, int mask, Tys... masks) noexcept
        {
            return mask_helper(acc | mask, (masks << 1)...);
        }
    };

    /**
     * @brief batch of integral constants
     *
     * Abstract representation of a batch of integral constants.
     *
     * @tparam batch_type the type of the associated batch values.
     * @tparam Values constants represented by this batch
     **/
    template <class batch_type, typename batch_type::value_type... Values>
    struct batch_constant
    {
        static constexpr std::size_t size = sizeof...(Values);
        using arch_type = typename batch_type::arch_type;
        using value_type = typename batch_type::value_type;
        static_assert(sizeof...(Values) == batch_type::size, "consistent batch size");

        /**
         * @brief Generate a batch of @p batch_type from this @p batch_constant
         */
        operator batch_type() const noexcept { return { Values... }; }

        /**
         * @brief Get the @p i th element of this @p batch_constant
         */
        constexpr value_type get(size_t i) const noexcept
        {
            return get(i, std::array<value_type, size> { Values... });
        }

    private:
        constexpr value_type get(size_t i, std::array<value_type, size> const& values) const noexcept
        {
            return values[i];
        }
    };

    namespace detail
    {
        template <class batch_type, class G, std::size_t... Is>
        inline constexpr auto make_batch_constant(detail::index_sequence<Is...>) noexcept
            -> batch_constant<batch_type, (typename batch_type::value_type)G::get(Is, sizeof...(Is))...>
        {
            return {};
        }
        template <class batch_type, class G, std::size_t... Is>
        inline constexpr auto make_batch_bool_constant(detail::index_sequence<Is...>) noexcept
            -> batch_bool_constant<batch_type, G::get(Is, sizeof...(Is))...>
        {
            return {};
        }

    } // namespace detail

    /**
     * @brief Build a @c batch_constant out of a generator function
     *
     * @tparam batch_type type of the (non-constant) batch to build
     * @tparam G type used to generate that batch. That type must have a static
     * member @c get that's used to generate the batch constant. Conversely, the
     * generated batch_constant has value `{G::get(0, batch_size), ... , G::get(batch_size - 1, batch_size)}`
     *
     * The following generator produces a batch of `(n - 1, 0, 1, ... n-2)`
     *
     * @code
     * struct Rot
     * {
     *     static constexpr unsigned get(unsigned i, unsigned n)
     *     {
     *         return (i + n - 1) % n;
     *     }
     * };
     * @endcode
     */
    template <class batch_type, class G>
    inline constexpr auto make_batch_constant() noexcept -> decltype(detail::make_batch_constant<batch_type, G>(detail::make_index_sequence<batch_type::size>()))
    {
        return detail::make_batch_constant<batch_type, G>(detail::make_index_sequence<batch_type::size>());
    }

    template <class batch_type, class G>
    inline constexpr auto make_batch_bool_constant() noexcept
        -> decltype(detail::make_batch_bool_constant<batch_type, G>(
            detail::make_index_sequence<batch_type::size>()))
    {
        return detail::make_batch_bool_constant<batch_type, G>(
            detail::make_index_sequence<batch_type::size>());
    }

} // namespace turbo::simd

#endif  // TURBO_SIMD_TYPES_BATCH_CONSTANT_H_

