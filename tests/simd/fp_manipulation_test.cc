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
struct fp_manipulation_test
{
    using batch_type = B;
    using arch_type = typename B::arch_type;
    using value_type = typename B::value_type;
    static constexpr size_t size = B::size;
    using array_type = std::array<value_type, size>;
    using int_value_type = turbo::simd::as_integer_t<value_type>;
    using int_batch_type = turbo::simd::batch<int_value_type, arch_type>;

    array_type input;
    int_value_type exponent;

    fp_manipulation_test()
    {
        exponent = 5;
        for (size_t i = 0; i < size; ++i)
        {
            input[i] = value_type(i) / 4 + value_type(1.2) * std::sqrt(value_type(i + 0.25));
        }
    }

    void test_fp_manipulations() const
    {
        int_batch_type bexp(exponent);
        // ldexp
        {
            array_type expected;
            std::transform(input.cbegin(), input.cend(), expected.begin(),
                           [this](const value_type& v)
                           { return std::ldexp(v, exponent); });
            batch_type res = turbo::simd::ldexp(batch_input(), bexp);
            INFO("ldexp");
            CHECK_BATCH_EQ(res, expected);
        }
        // frexp
        {
            array_type expected;
            std::transform(input.cbegin(), input.cend(), expected.begin(),
                           [](const value_type& v)
                           { int tmp; return std::frexp(v, &tmp); });
            batch_type res = turbo::simd::frexp(batch_input(), bexp);
            INFO("frexp");
            CHECK_BATCH_EQ(res, expected);
        }
    }

private:
    batch_type batch_input() const
    {
        return batch_type::load_unaligned(input.data());
    }
};

TEST_CASE_TEMPLATE("[fp manipulation]", B, BATCH_FLOAT_TYPES)
{
    fp_manipulation_test<B> Test;
    Test.test_fp_manipulations();
}
#endif
