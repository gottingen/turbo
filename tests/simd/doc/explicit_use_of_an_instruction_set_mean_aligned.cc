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
#include <cstddef>
#include <vector>

using vector_type = std::vector<double, turbo::simd::default_allocator<double>>;

void mean(const vector_type &a, const vector_type &b, vector_type &res) {
    using b_type = turbo::simd::batch<double>;
    std::size_t inc = b_type::size;
    std::size_t size = res.size();
    // size for which the vectorization is possible
    std::size_t vec_size = size - size % inc;
    for (std::size_t i = 0; i < vec_size; i += inc) {
        b_type avec = b_type::load_aligned(&a[i]);
        b_type bvec = b_type::load_aligned(&b[i]);
        b_type rvec = (avec + bvec) / 2;
        rvec.store_aligned(&res[i]);
    }
    // Remaining part that cannot be vectorize
    for (std::size_t i = vec_size; i < size; ++i) {
        res[i] = (a[i] + b[i]) / 2;
    }
}
