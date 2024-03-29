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

#if !TURBO_WITH_NEON || TURBO_WITH_NEON64
template <class CP>
struct conversion_test
{
    static constexpr size_t N = CP::size;
    static constexpr size_t A = CP::alignment;

    using int32_batch = turbo::simd::batch<int32_t>;
    using int64_batch = turbo::simd::batch<int64_t>;
    using float_batch = turbo::simd::batch<float>;
    using double_batch = turbo::simd::batch<double>;

    using uint8_batch = turbo::simd::batch<uint8_t>;
    using uint16_batch = turbo::simd::batch<uint16_t>;
    using uint32_batch = turbo::simd::batch<uint32_t>;
    using uint64_batch = turbo::simd::batch<uint64_t>;

    using int32_vector = std::vector<int32_t, turbo::simd::default_allocator<int32_t>>;
    using int64_vector = std::vector<int64_t, turbo::simd::default_allocator<int64_t>>;
    using float_vector = std::vector<float, turbo::simd::default_allocator<float>>;
    using double_vector = std::vector<double, turbo::simd::default_allocator<double>>;

    using uint8_vector = std::vector<uint8_t, turbo::aligned_allocator<uint8_t, A>>;

    /*int32_batch i32pos;
    int32_batch i32neg;
    int64_batch i64pos;
    int64_batch i64neg;
    float_batch fpos;
    float_batch fneg;
    double_batch dpos;
    double_batch dneg;*/

    int32_vector fposres;
    int32_vector fnegres;
    int64_vector dposres;
    int64_vector dnegres;
    float_vector i32posres;
    float_vector i32negres;
    double_vector i64posres;
    double_vector i64negres;

    uint8_vector ui8res;

    conversion_test()
        : fposres(2 * N, 7)
        , fnegres(2 * N, -6)
        , dposres(N, 5)
        , dnegres(N, -1)
        , i32posres(2 * N, float(2))
        , i32negres(2 * N, float(-3))
        , i64posres(N, double(2))
        , i64negres(N, double(-3))
        , ui8res(8 * N, 4)
    {
    }

    void test_to_int32()
    {
        float_batch fpos(float(7.4)), fneg(float(-6.2));
        int32_vector fvres(int32_batch::size);
        {
            int32_batch fbres = to_int(fpos);
            fbres.store_aligned(fvres.data());
            INFO("to_int(positive float)");
            CHECK_VECTOR_EQ(fvres, fposres);
        }
        {
            int32_batch fbres = to_int(fneg);
            fbres.store_aligned(fvres.data());
            INFO("to_int(negative float)");
            CHECK_VECTOR_EQ(fvres, fnegres);
        }
    }

    void test_to_int64()
    {
        double_batch dpos(double(5.4)), dneg(double(-1.2));
        int64_vector dvres(int64_batch::size);
        {
            int64_batch dbres = to_int(dpos);
            dbres.store_aligned(dvres.data());
            INFO("to_int(positive double)");
            CHECK_VECTOR_EQ(dvres, dposres);
        }
        {
            int64_batch dbres = to_int(dneg);
            dbres.store_aligned(dvres.data());
            INFO("to_int(negative double)");
            CHECK_VECTOR_EQ(dvres, dnegres);
        }
    }

    void test_to_float()
    {
        int32_batch i32pos(2), i32neg(-3);
        float_vector i32vres(float_batch::size);
        {
            float_batch i32bres = to_float(i32pos);
            i32bres.store_aligned(i32vres.data());
            INFO("to_float(positive int32)");
            CHECK_VECTOR_EQ(i32vres, i32posres);
        }
        {
            float_batch i32bres = to_float(i32neg);
            i32bres.store_aligned(i32vres.data());
            INFO("to_float(negative int32)");
            CHECK_VECTOR_EQ(i32vres, i32negres);
        }
    }

    void test_to_double()
    {
        int64_batch i64pos(2), i64neg(-3);
        double_vector i64vres(double_batch::size);
        {
            double_batch i64bres = to_float(i64pos);
            i64bres.store_aligned(i64vres.data());
            INFO("to_float(positive int64)");
            CHECK_VECTOR_EQ(i64vres, i64posres);
        }
        {
            double_batch i64bres = to_float(i64neg);
            i64bres.store_aligned(i64vres.data());
            INFO("to_float(negative int64)");
            CHECK_VECTOR_EQ(i64vres, i64negres);
        }
    }

    void test_u8_casting()
    {
        uint8_batch ui8tmp(4);
        uint8_vector ui8vres(uint8_batch::size);
        {
            uint16_batch ui16casting = turbo::simd::bitwise_cast<uint16_t>(ui8tmp);
            uint8_batch ui8casting = turbo::simd::bitwise_cast<uint8_t>(ui16casting);
            ui8casting.store_aligned(ui8vres.data());
            INFO("u8_to_16");
            CHECK_VECTOR_EQ(ui8vres, ui8res);
        }
        {
            uint32_batch ui32casting = turbo::simd::bitwise_cast<uint32_t>(ui8tmp);
            uint8_batch ui8casting = turbo::simd::bitwise_cast<uint8_t>(ui32casting);
            ui8casting.store_aligned(ui8vres.data());
            INFO("u8_to_32");
            CHECK_VECTOR_EQ(ui8vres, ui8res);
        }
        {
            uint64_batch ui64casting = turbo::simd::bitwise_cast<uint64_t>(ui8tmp);
            uint8_batch ui8casting = turbo::simd::bitwise_cast<uint8_t>(ui64casting);
            ui8casting.store_aligned(ui8vres.data());
            INFO("u8_to_64");
            CHECK_VECTOR_EQ(ui8vres, ui8res);
        }
    }
};

TEST_CASE_TEMPLATE("[conversion]", B, CONVERSION_TYPES)
{
    conversion_test<B> Test;

    SUBCASE("to_int32")
    {
        Test.test_to_int32();
    }

    SUBCASE("to_int64")
    {
        Test.test_to_int64();
    }

    SUBCASE("to_float")
    {
        Test.test_to_float();
    }

    SUBCASE("to_double")
    {
        Test.test_to_double();
    }

    SUBCASE("u8_casting")
    {
        Test.test_u8_casting();
    }
}

template <class T>
struct sign_conversion_test
{

    using unsigned_type = T;
    using signed_type = typename std::make_signed<T>::type;

    void test_to_signed()
    {
        unsigned_type unsigned_value = 3;
        signed_type signed_value = (signed_type)unsigned_value;
        turbo::simd::batch<unsigned_type> unsigned_batch(unsigned_value);
        auto signed_batch = turbo::simd::batch_cast<signed_type>(unsigned_batch);
        CHECK_EQ(unsigned_batch.get(0), unsigned_value);
        CHECK_EQ(signed_batch.get(0), signed_value);
    }

    void test_to_unsigned()
    {
        signed_type signed_value = 3;
        unsigned_type unsigned_value = (unsigned_type)signed_value;
        turbo::simd::batch<signed_type> signed_batch(signed_value);
        auto unsigned_batch = turbo::simd::batch_cast<unsigned_type>(signed_batch);
        CHECK_EQ(signed_batch.get(0), signed_value);
        CHECK_EQ(unsigned_batch.get(0), unsigned_value);
    }
};

TEST_CASE_TEMPLATE("[conversion]", T, uint8_t, uint16_t, uint32_t, uint64_t)
{
    sign_conversion_test<T> Test;

    SUBCASE("to_signed")
    {
        Test.test_to_signed();
    }

    SUBCASE("to_unsigned")
    {
        Test.test_to_unsigned();
    }
}

#endif
#endif
