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
// Created by jeff on 24-1-6.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"
#include "turbo/container/small_vector.h"

TEST_CASE("SmallVector" * doctest::timeout(300)) {

    //SUBCASE("constructor")
    {
        turbo::SmallVector<int> vec1;
        REQUIRE(vec1.size() == 0);
        REQUIRE(vec1.empty() == true);

        turbo::SmallVector<int, 4> vec2;
        REQUIRE(vec2.data() != nullptr);
        REQUIRE(vec2.size() == 0);
        REQUIRE(vec2.empty() == true);
        REQUIRE(vec2.capacity() == 4);
    }

    //SUBCASE("constructor_n")
    {
        for (int N = 0; N <= 65536; N = (N ? N << 1 : 1)) {
            turbo::SmallVector<int> vec(N);
            REQUIRE(vec.size() == N);
            REQUIRE(vec.empty() == (N == 0));
            REQUIRE(vec.max_size() >= vec.size());
            REQUIRE(vec.capacity() >= vec.size());
        }
    }

    //SUBCASE("copy_constructor")
    {
        for (int N = 0; N <= 65536; N = (N ? N << 1 : 1)) {
            turbo::SmallVector<int> vec1(N);
            for (auto &item: vec1) {
                item = N;
            }

            turbo::SmallVector<int> vec2(vec1);
            REQUIRE(vec1.size() == N);
            REQUIRE(vec2.size() == N);
            for (size_t i = 0; i < vec1.size(); ++i) {
                REQUIRE(vec1[i] == vec2[i]);
                REQUIRE(vec1[i] == N);
            }
        }
    }

    //SUBCASE("move_constructor")
    {
        for (int N = 0; N <= 65536; N = (N ? N << 1 : 1)) {
            turbo::SmallVector<int> vec1(N);
            for (auto &item: vec1) {
                item = N;
            }

            turbo::SmallVector<int> vec2(std::move(vec1));
            REQUIRE(vec1.size() == 0);
            REQUIRE(vec1.empty() == true);
            REQUIRE(vec2.size() == N);

            for (size_t i = 0; i < vec2.size(); ++i) {
                REQUIRE(vec2[i] == N);
            }
        }
    }

    //SUBCASE("push_back")
    {
        for (int N = 0; N <= 65536; N = (N ? N << 1 : 1)) {
            turbo::SmallVector<int> vec;
            size_t pcap{0};
            size_t ncap{0};
            for (int n = 0; n < N; ++n) {
                vec.push_back(n);
                REQUIRE(vec.size() == n + 1);
                ncap = vec.capacity();
                REQUIRE(ncap >= pcap);
                pcap = ncap;
            }
            for (int n = 0; n < N; ++n) {
                REQUIRE(vec[n] == n);
            }
            REQUIRE(vec.empty() == (N == 0));
        }
    }

    //SUBCASE("pop_back")
    {
        size_t size{0};
        size_t pcap{0};
        size_t ncap{0};
        turbo::SmallVector<int> vec;
        for (int N = 0; N <= 65536; N = (N ? N << 1 : N + 1)) {
            vec.push_back(N);
            ++size;
            REQUIRE(vec.size() == size);
            if (N % 4 == 0) {
                vec.pop_back();
                --size;
                REQUIRE(vec.size() == size);
            }
            ncap = vec.capacity();
            REQUIRE(ncap >= pcap);
            pcap = ncap;
        }
        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < vec.size(); ++i) {
            REQUIRE(vec[i] % 4 != 0);
        }
    }

    //SUBCASE("iterator")
    {
        for (int N = 0; N <= 65536; N = (N ? N << 1 : 1)) {
            turbo::SmallVector<int> vec;
            for (int n = 0; n < N; ++n) {
                vec.push_back(n);
                REQUIRE(vec.size() == n + 1);
            }

            // non-constant iterator
            {
                int val{0};
                for (auto item: vec) {
                    REQUIRE(item == val);
                    ++val;
                }
            }

            // constant iterator
            {
                int val{0};
                for (const auto &item: vec) {
                    REQUIRE(item == val);
                    ++val;
                }
            }

            // change the value
            {
                for (auto &item: vec) {
                    item = 1234;
                }
                for (auto &item: vec) {
                    REQUIRE(item == 1234);
                }
            }
        }
    }

    //SUBCASE("clear")
    {
        for (int N = 0; N <= 65536; N = (N ? N << 1 : 1)) {
            turbo::SmallVector<int> vec(N);
            auto cap = vec.capacity();
            REQUIRE(vec.size() == N);
            vec.clear();
            REQUIRE(vec.size() == 0);
            REQUIRE(vec.capacity() == cap);
        }
    }

    //SUBCASE("comparison")
    {
        for (int N = 0; N <= 65536; N = (N ? N << 1 : 1)) {
            turbo::SmallVector<int> vec1;
            for (int i = 0; i < N; ++i) {
                vec1.push_back(i);
            }
            turbo::SmallVector<int> vec2(vec1);
            REQUIRE(vec1 == vec2);
        }
    }
}

