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
            explicit_use_of_an_instruction_set_mean_aligned,cc
            explicit_use_of_an_instruction_set_mean_arch_independent,cc
            explicit_use_of_an_instruction_set_mean,cc
            explicit_use_of_an_instruction_set_mean_tag_dispatch,cc
            manipulating_abstract_batches,cc
            manipulating_parametric_batches,cc
            writing_vectorized_code,cc)
target_include_directories(test_doc_any_arch PRIVATE ${TURBO_SIMD_INCLUDE_DIR})
target_compile_options(test_doc_any_arch PRIVATE -mavx)

add_library(test_doc_avx2 OBJECT
            explicit_use_of_an_instruction_set,cc
            sum_avx2,cc)
target_compile_options(test_doc_avx2 PRIVATE -mavx2)
target_include_directories(test_doc_avx2 PRIVATE ${TURBO_SIMD_INCLUDE_DIR})

add_library(test_doc_sse2 OBJECT
            sum_sse2,cc)
target_compile_options(test_doc_sse2 PRIVATE -msse2)
target_include_directories(test_doc_sse2 PRIVATE ${TURBO_SIMD_INCLUDE_DIR})

add_dependencies(xtest test_doc_any_arch test_doc_avx2 test_doc_sse2)

endif()
