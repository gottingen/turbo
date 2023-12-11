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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "turbo/hash/mixer.h"
#include "turbo/hash/hash.h"
#include "turbo/format/print.h"

struct HashTest {
    int a;
    template <typename H>
    H hash_value(H state, const HashTest& v) {
        return H::combine(std::move(state), v.a);
    }
};

namespace std {
    template<>
    struct hash<HashTest> {
        size_t operator()(const HashTest &h) const {
            return h.a;
        }
    };
}

namespace turbo {

    TEST_CASE("mix, try") {
        auto r = hash_mixer<4>()(2);
        turbo::Println("r: {}", static_cast<uint32_t>(r));
        REQUIRE_NE(r,  0);
        r = hash_mixer<8>()(2);
        turbo::Println("r: {}", r);
        REQUIRE_NE(r,  0);

        auto rr = hash_mixer4<uint32_t>(2);
        turbo::Println("rr: {}", rr);
        size_t ir = hash_mixer4<simple_mix>(2);
        turbo::Println("ir: {}", ir);
    }

    TEST_CASE("mix, murmur") {
        auto r = hash_mixer<4, size_t, murmur_mix>()(2);
        turbo::Println("murmur r: {}", static_cast<uint32_t>(r));
        REQUIRE_NE(r,  0);
        r = hash_mixer<8, size_t, murmur_mix>()(2);
        turbo::Println("murmur r: {}", r);
        REQUIRE_NE(r,  0);

        auto rr = hash_mixer4<uint32_t,murmur_mix>(2);
        turbo::Println("murmur rr: {}", rr);
        size_t ir = hash_mixer4<murmur_mix>(2);
        turbo::Println("murmur ir: {}", ir);
    }

    TEST_CASE("hash, murmur") {
        HashTest aa;
        aa.a = 3;
        turbo::Println("std r: {}", std::hash<HashTest>()(aa));

        turbo::Println("bytes engine: {}", turbo::Hash<HashTest>()(aa));
        turbo::Println("bytes engine: {}", turbo::Hash<int>{}(3));

        turbo::Println("m3 engine: {}", turbo::Hash<HashTest,m3_hash_tag>{}(aa));
        turbo::Println("m3 engine: {}", turbo::Hash<int, m3_hash_tag>{}(3));

        turbo::Println("xx engine: {}", turbo::Hash<int, xx_hash_tag>{}(3));
        turbo::Println("xx engine: {}", turbo::Hash<HashTest, xx_hash_tag>{}(aa));

        turbo::Println("city engine: {}", turbo::Hash<int, city_hash_tag>{}(3));
        turbo::Println("city engine: {}", turbo::Hash<HashTest, city_hash_tag>{}(aa));
    }
}
