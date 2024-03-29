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

namespace turbo::simd
{
    template <typename T, std::size_t N>
    struct init_extract_pair_base
    {
        using extract_vector_type = std::array<T, N>;
        extract_vector_type lhs_in, rhs_in, exped;

        std::vector<extract_vector_type> create_extract_vectors(const int index)
        {
            std::vector<extract_vector_type> vects;
            vects.reserve(3);

            int num = static_cast<int>(N);
            /* Generate input data: lhs, rhs */
            for (int i = 0; i < num; ++i)
            {
                lhs_in[i] = 2 * i + 1;
                rhs_in[i] = 2 * i + 2;
            }
            vects.push_back(std::move(lhs_in));
            vects.push_back(std::move(rhs_in));

            /* Expected shuffle data */
            for (int i = 0; i < (num - index); ++i)
            {
                exped[i] = rhs_in[i + index];
                if (i < index)
                {
                    exped[num - 1 - i] = lhs_in[index - 1 - i];
                }
            }
            vects.push_back(std::move(exped));

            return vects;
        }
    };
}

template <class B>
struct extract_pair_test
{
    using batch_type = B;
    using value_type = typename B::value_type;
    static constexpr size_t size = B::size;

    void extract_pair_128()
    {
        turbo::simd::init_extract_pair_base<value_type, size> extract_pair_base;
        auto extract_pair_vecs = extract_pair_base.create_extract_vectors(1);
        auto v_lhs = extract_pair_vecs[0];
        auto v_rhs = extract_pair_vecs[1];
        auto v_exped = extract_pair_vecs[2];

        B b_lhs = B::load_unaligned(v_lhs.data());
        B b_rhs = B::load_unaligned(v_rhs.data());
        B b_exped = B::load_unaligned(v_exped.data());

        /* Only Test 128bit */
        if ((sizeof(value_type) * size) == 16)
        {
            B b_res = turbo::simd::extract_pair(b_lhs, b_rhs, 1);
            CHECK_BATCH_EQ(b_res, b_exped);
        }
    }
};

TEST_CASE_TEMPLATE("[extract pair]", B, BATCH_TYPES)
{
    extract_pair_test<B> Test;
    SUBCASE("extract_pair_128")
    {
        Test.extract_pair_128();
    }
}
#endif
