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

#include "turbo/simd/simd.h"
#ifndef TURBO_NO_SUPPORTED_ARCHITECTURE

#include "test_utils.h"

template <class B>
struct error_gamma_test
{
    using batch_type = B;
    using value_type = typename B::value_type;
    static constexpr size_t size = B::size;
    using vector_type = std::vector<value_type>;

    size_t nb_input;
    vector_type input;
    vector_type gamma_input;
    vector_type gamma_neg_input;
    vector_type expected;
    vector_type res;

    error_gamma_test()
    {
        nb_input = size * 10000;
        input.resize(nb_input);
        gamma_input.resize(nb_input);
        gamma_neg_input.resize(nb_input);
        for (size_t i = 0; i < nb_input; ++i)
        {
            input[i] = value_type(-1.5) + i * value_type(3) / nb_input;
            gamma_input[i] = value_type(0.5) + i * value_type(3) / nb_input;
            gamma_neg_input[i] = value_type(-3.99) + i * value_type(0.9) / nb_input;
        }
        expected.resize(nb_input);
        res.resize(nb_input);
    }

    void test_error_functions()
    {
        // erf
        {
            std::transform(input.cbegin(), input.cend(), expected.begin(),
                           [](const value_type& v)
                           { return std::erf(v); });
            batch_type in, out;
            for (size_t i = 0; i < nb_input; i += size)
            {
                detail::load_batch(in, input, i);
                out = erf(in);
                detail::store_batch(out, res, i);
            }
            size_t diff = detail::get_nb_diff(res, expected);
            INFO("erf");
            CHECK_EQ(diff, 0);
        }
        // erfc
        {
            std::transform(input.cbegin(), input.cend(), expected.begin(),
                           [](const value_type& v)
                           { return std::erfc(v); });
            batch_type in, out;
            for (size_t i = 0; i < nb_input; i += size)
            {
                detail::load_batch(in, input, i);
                out = erfc(in);
                detail::store_batch(out, res, i);
            }
            size_t diff = detail::get_nb_diff(res, expected);
            INFO("erfc");
            CHECK_EQ(diff, 0);
        }
    }

    void test_gamma_functions()
    {
        // tgamma
        {
            std::transform(gamma_input.cbegin(), gamma_input.cend(), expected.begin(),
                           [](const value_type& v)
                           { return std::tgamma(v); });
            batch_type in, out;
            for (size_t i = 0; i < nb_input; i += size)
            {
                detail::load_batch(in, gamma_input, i);
                out = tgamma(in);
                detail::store_batch(out, res, i);
            }
            size_t diff = detail::get_nb_diff(res, expected);
            INFO("tgamma");
            CHECK_EQ(diff, 0);
        }
        // tgamma (negative input)
        {
            std::transform(gamma_neg_input.cbegin(), gamma_neg_input.cend(), expected.begin(),
                           [](const value_type& v)
                           { return std::tgamma(v); });
            batch_type in, out;
            for (size_t i = 0; i < nb_input; i += size)
            {
                detail::load_batch(in, gamma_neg_input, i);
                out = tgamma(in);
                detail::store_batch(out, res, i);
            }
            size_t diff = detail::get_nb_diff(res, expected);
            INFO("tgamma (negative input)");
            CHECK_EQ(diff, 0);
        }
        // lgamma
        {
            std::transform(gamma_input.cbegin(), gamma_input.cend(), expected.begin(),
                           [](const value_type& v)
                           { return std::lgamma(v); });
            batch_type in, out;
            for (size_t i = 0; i < nb_input; i += size)
            {
                detail::load_batch(in, gamma_input, i);
                out = lgamma(in);
                detail::store_batch(out, res, i);
            }
            size_t diff = detail::get_nb_diff(res, expected);
            INFO("lgamma");
            CHECK_EQ(diff, 0);
        }
#if !(TURBO_WITH_AVX && !TURBO_WITH_AVX2)

        // tgamma (negative input)
        {
            std::transform(gamma_neg_input.cbegin(), gamma_neg_input.cend(), expected.begin(),
                           [](const value_type& v)
                           { return std::lgamma(v); });
            batch_type in, out;
            for (size_t i = 0; i < nb_input; i += size)
            {
                detail::load_batch(in, gamma_neg_input, i);
                out = lgamma(in);
                detail::store_batch(out, res, i);
            }
            size_t diff = detail::get_nb_diff(res, expected);
            INFO("lgamma (negative input)");
            CHECK_EQ(diff, 0);
        }
#endif
    }
};

TEST_CASE_TEMPLATE("[error gamma]", B, BATCH_FLOAT_TYPES)
{
    error_gamma_test<B> Test;

    SUBCASE("error")
    {
        Test.test_error_functions();
    }

    SUBCASE("gamma")
    {
        Test.test_gamma_functions();
    }
}
#endif
