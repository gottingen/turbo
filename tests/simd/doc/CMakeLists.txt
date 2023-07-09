#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Only test under some architecture, because it's just a sanity check, no full
# coverage is needed.

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" AND NOT CMAKE_OSX_ARCHITECTURES)

add_library(test_doc_any_arch OBJECT
            explicit_use_of_an_instruction_set_mean_aligned.cpp
            explicit_use_of_an_instruction_set_mean_arch_independent.cpp
            explicit_use_of_an_instruction_set_mean.cpp
            explicit_use_of_an_instruction_set_mean_tag_dispatch.cpp
            manipulating_abstract_batches.cpp
            manipulating_parametric_batches.cpp
            writing_vectorized_code.cpp)
target_include_directories(test_doc_any_arch PRIVATE ${TURBO_SIMD_INCLUDE_DIR})
target_compile_options(test_doc_any_arch PRIVATE -mavx)

add_library(test_doc_avx2 OBJECT
            explicit_use_of_an_instruction_set.cpp
            sum_avx2.cpp)
target_compile_options(test_doc_avx2 PRIVATE -mavx2)
target_include_directories(test_doc_avx2 PRIVATE ${TURBO_SIMD_INCLUDE_DIR})

add_library(test_doc_sse2 OBJECT
            sum_sse2.cpp)
target_compile_options(test_doc_sse2 PRIVATE -msse2)
target_include_directories(test_doc_sse2 PRIVATE ${TURBO_SIMD_INCLUDE_DIR})

add_dependencies(xtest test_doc_any_arch test_doc_avx2 test_doc_sse2)

endif()
