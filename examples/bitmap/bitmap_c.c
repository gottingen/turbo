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
#include <stdio.h>
#include <stdlib.h>
#include <turbo/bitmap/roaring.h>

int main() {
    roaring_bitmap_t *r1 = roaring_bitmap_create();
    for (uint32_t i = 100; i < 1000; i++) roaring_bitmap_add(r1, i);
    printf("cardinality = %d\n", (int) roaring_bitmap_get_cardinality(r1));
    roaring_bitmap_free(r1);

    bitset_t *b = bitset_create();
    for (int k = 0; k < 1000; ++k) {
        bitset_set(b, 3 * k);
    }
    printf("%zu \n", bitset_count(b));
    bitset_free(b);
    return EXIT_SUCCESS;
}