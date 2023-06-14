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


#include "turbo/simd/simd.h"
#ifndef TURBO_NO_SUPPORTED_ARCHITECTURE

#include "test_utils.h"

template <class B>
struct traits_test
{
    using batch_type = B;
    using value_type = typename B::value_type;

    void test_simd_traits()
    {
        using traits_type = turbo::simd::simd_traits<value_type>;
        CHECK_EQ(traits_type::size, batch_type::size);
        constexpr bool same_type = std::is_same<B, typename traits_type::type>::value;
        CHECK_UNARY(same_type);
        using batch_bool_type = turbo::simd::batch_bool<value_type>;
        constexpr bool same_bool_type = std::is_same<batch_bool_type, typename traits_type::bool_type>::value;
        CHECK_UNARY(same_bool_type);

        using vector_traits_type = turbo::simd::simd_traits<std::vector<value_type>>;
        CHECK_EQ(vector_traits_type::size, 1);
        constexpr bool vec_same_type = std::is_same<typename vector_traits_type::type, std::vector<value_type>>::value;
        CHECK_UNARY(vec_same_type);
    }

    void test_revert_simd_traits()
    {
        using traits_type = turbo::simd::revert_simd_traits<batch_type>;
        CHECK_EQ(traits_type::size, batch_type::size);
        constexpr bool same_type = std::is_same<value_type, typename traits_type::type>::value;
        CHECK_UNARY(same_type);
    }

    void test_simd_return_type()
    {
        using rtype1 = turbo::simd::simd_return_type<value_type, float>;
        constexpr bool res1 = std::is_same<rtype1, turbo::simd::batch<float>>::value;
        CHECK_UNARY(res1);

        using rtype2 = turbo::simd::simd_return_type<bool, value_type>;
        constexpr bool res2 = std::is_same<rtype2, turbo::simd::batch_bool<value_type>>::value;
        CHECK_UNARY(res2);
    }

    void test_mask_type()
    {
        using mtype0 = turbo::simd::mask_type_t<batch_type>;
        constexpr bool res0 = std::is_same<mtype0, turbo::simd::batch_bool<turbo::simd::scalar_type_t<batch_type>>>::value;
        CHECK_UNARY(res0);

        using mtype1 = turbo::simd::mask_type_t<value_type>;
        constexpr bool res1 = std::is_same<mtype1, bool>::value;
        CHECK_UNARY(res1);
    }
};

TEST_CASE_TEMPLATE("[traits]", B, BATCH_TYPES)
{
    traits_test<B> Test;

    SUBCASE("simd_traits")
    {
        Test.test_simd_traits();
    }

    SUBCASE("revert_simd_traits")
    {
        Test.test_revert_simd_traits();
    }

    SUBCASE("simd_return_type")
    {
        Test.test_simd_return_type();
    }

    SUBCASE("mask_type")
    {
        Test.test_mask_type();
    }
}

template <class B>
struct complex_traits_test
{
    using batch_type = B;
    using value_type = typename B::value_type;

    void test_simd_traits()
    {
        using traits_type = turbo::simd::simd_traits<value_type>;
        CHECK_EQ(traits_type::size, batch_type::size);
        constexpr bool same_type = std::is_same<B, typename traits_type::type>::value;
        CHECK_UNARY(same_type);
        using batch_bool_type = turbo::simd::batch_bool<typename value_type::value_type>;
        constexpr bool same_bool_type = std::is_same<batch_bool_type, typename traits_type::bool_type>::value;
        CHECK_UNARY(same_bool_type);

        using vector_traits_type = turbo::simd::simd_traits<std::vector<value_type>>;
        CHECK_EQ(vector_traits_type::size, 1);
        constexpr bool vec_same_type = std::is_same<typename vector_traits_type::type, std::vector<value_type>>::value;
        CHECK_UNARY(vec_same_type);
    }

    void test_revert_simd_traits()
    {
        using traits_type = turbo::simd::revert_simd_traits<batch_type>;
        CHECK_EQ(traits_type::size, batch_type::size);
        constexpr bool same_type = std::is_same<value_type, typename traits_type::type>::value;
        CHECK_UNARY(same_type);
    }

    void test_simd_return_type()
    {
        using rtype1 = turbo::simd::simd_return_type<value_type, float>;
        constexpr bool res1 = std::is_same<rtype1, turbo::simd::batch<std::complex<float>>>::value;
        CHECK_UNARY(res1);

        using rtype2 = turbo::simd::simd_return_type<bool, value_type>;
        constexpr bool res2 = std::is_same<rtype2, turbo::simd::batch_bool<typename value_type::value_type>>::value;
        CHECK_UNARY(res2);
    }

    void test_mask_type()
    {
        using mtype0 = turbo::simd::mask_type_t<batch_type>;
        constexpr bool res0 = std::is_same<mtype0, turbo::simd::batch_bool<turbo::simd::scalar_type_t<typename batch_type::real_batch::value_type>>>::value;
        CHECK_UNARY(res0);

        using mtype1 = turbo::simd::mask_type_t<value_type>;
        constexpr bool res1 = std::is_same<mtype1, bool>::value;
        CHECK_UNARY(res1);
    }
};

TEST_CASE_TEMPLATE("[complex traits]", B, BATCH_COMPLEX_TYPES)
{
    complex_traits_test<B> Test;

    SUBCASE("simd_traits")
    {
        Test.test_simd_traits();
    }

    SUBCASE("revert_simd_traits")
    {
        Test.test_revert_simd_traits();
    }

    SUBCASE("simd_return_type")
    {
        Test.test_simd_return_type();
    }

    SUBCASE("mask_type")
    {
        Test.test_mask_type();
    }
}
#endif
