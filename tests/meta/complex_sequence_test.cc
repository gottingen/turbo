// Copyright 2023 The titan-search Authors.
//
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
#include "turbo/meta/complex_sequence.h"
#include "tests/doctest/test_common_macros.h"

namespace turbo {

    TEST(xcomplex_sequence, constructor)
    {
        complex_vector<double, false> v1;
        EXPECT_TRUE(v1.empty());
        EXPECT_EQ(v1.size(), std::size_t(0));

        complex_vector<double, false> v2(10);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(v2.size(), std::size_t(10));

        complex_vector<double, false> v3(10, complex<double>(1., 1.));
        EXPECT_FALSE(v3.empty());
        EXPECT_EQ(v3.size(), std::size_t(10));
    }

    TEST(xcomplex_sequence, resize)
    {
        complex_vector<double, false> v1(2);
        v1.resize(4);
        EXPECT_EQ(v1.size(), 4u);

        v1.resize(8, complex<double>(1., 1.));
        EXPECT_EQ(v1.size(), 8u);
    }

    TEST(xcomplex_sequence, access)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);
        
        complex_vector<double, false> v = { c0, c1 };
        EXPECT_EQ(v[0], c0);
        EXPECT_EQ(v[1], c1);
    }

    TEST(xcomplex_sequence, at)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);

        complex_vector<double, false> v = { c0, c1 };
        EXPECT_EQ(v.at(0), c0);
        EXPECT_EQ(v.at(1), c1);
    }

    TEST(xcomplex_sequence, front)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);

        complex_vector<double, false> v = { c0, c1 };
        EXPECT_EQ(v.front(), c0);
    }

    TEST(xcomplex_sequence, back)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);

        complex_vector<double, false> v = { c0, c1 };
        EXPECT_EQ(v.back(), c1);
    }

    TEST(xcomplex_sequence, iterator)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);
        complex<double> c2(4., 6.);
        complex<double> c3(8., 12.);

        complex_vector<double, false> v = { c0, c1, c2, c3 };
        auto iter = v.begin();
        auto citer = v.cbegin();

        EXPECT_EQ(*iter, c0);
        EXPECT_EQ(*citer, c0);

        ++iter, ++citer;
        EXPECT_EQ(*iter, c1);
        EXPECT_EQ(*citer, c1);

        ++iter, ++citer;
        EXPECT_EQ(*iter, c2);
        EXPECT_EQ(*citer, c2);

        ++iter, ++citer;
        EXPECT_EQ(*iter, c3);
        EXPECT_EQ(*citer, c3);

        ++iter, ++citer;
        EXPECT_EQ(iter, v.end());
        EXPECT_EQ(citer, v.cend());
    }

    TEST(xcomplex_sequence, reverse_iterator)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);
        complex<double> c2(4., 6.);
        complex<double> c3(8., 12.);

        complex_vector<double, false> v = { c0, c1, c2, c3 };
        auto iter = v.rbegin();
        auto citer = v.crbegin();

        EXPECT_EQ(*iter, c3);
        EXPECT_EQ(*citer, c3);

        ++iter, ++citer;
        EXPECT_EQ(*iter, c2);
        EXPECT_EQ(*citer, c2);

        ++iter, ++citer;
        EXPECT_EQ(*iter, c1);
        EXPECT_EQ(*citer, c1);

        ++iter, ++citer;
        EXPECT_EQ(*iter, c0);
        EXPECT_EQ(*citer, c0);

        ++iter, ++citer;
        EXPECT_EQ(iter, v.rend());
        EXPECT_EQ(citer, v.crend());
    }

    TEST(xcomplex_sequence, comparison)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);
        complex<double> c2(4., 6.);
        complex<double> c3(8., 12.);
        complex_vector<double, false> v1 = { c0, c1, c2, c3 };

        EXPECT_TRUE(v1 == v1);
        EXPECT_FALSE(v1 != v1);
    }

    TEST(xcomplex_sequence, real)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);

        complex_vector<double, false> v = { c0, c1 };
        EXPECT_EQ(v.real()[0], 1.);
        EXPECT_EQ(v.real()[1], 2.);
    }

    TEST(xcomplex_sequence, imag)
    {
        complex<double> c0(1., 2.);
        complex<double> c1(2., 3.);

        complex_vector<double, false> v = { c0, c1 };
        EXPECT_EQ(v.imag()[0], 2.);
        EXPECT_EQ(v.imag()[1], 3.);
    }
}
