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
// Created by jeff on 24-4-26.
//
#include <iostream>
#include <turbo/bitmap/bitmap.h> // the amalgamated roaring.hh includes roaring64map.hh

int main() {
    turbo::Roaring r1;
    for (uint32_t i = 100; i < 1000; i++) {
        r1.add(i);
    }
    std::cout << "cardinality = " << r1.cardinality() << std::endl;

    turbo::Roaring64Map r2;
    for (uint64_t i = 18000000000000000100ull; i < 18000000000000001000ull; i++) {
        r2.add(i);
    }
    std::cout << "cardinality = " << r2.cardinality() << std::endl;
    return 0;
}