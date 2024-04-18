#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


carbin_cc_test(
        NAME
        random_bit_gen_ref_test
        MODULE random
        SOURCES
        "bit_gen_ref_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        random_mock_distributions_test
        MODULE random
        SOURCES
        "mock_distributions_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_mocking_bit_gen_test
        MODULE random
        SOURCES
        "mocking_bit_gen_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        random_bernoulli_distribution_test
        MODULE random
        SOURCES
        "bernoulli_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        random_beta_distribution_test
        MODULE random
        SOURCES
        "beta_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_distributions_test
        MODULE random
        SOURCES
        "distributions_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_generators_test
        MODULE random
        SOURCES
        "generators_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_log_uniform_int_distribution_test
        MODULE random
        SOURCES
        "log_uniform_int_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_discrete_distribution_test
        MODULE random
        SOURCES
        "discrete_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        pthread
        m
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_poisson_distribution_test
        MODULE random
        SOURCES
        "poisson_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_exponential_distribution_test
        MODULE random
        SOURCES
        "exponential_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_gaussian_distribution_test
        MODULE random
        SOURCES
        "gaussian_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        -lpthread
        -lm
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_uniform_int_distribution_test
        MODULE random
        SOURCES
        "uniform_int_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
)

carbin_cc_test(
        NAME
        random_uniform_real_distribution_test
        MODULE random
        SOURCES
        "uniform_real_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_zipf_distribution_test
        MODULE random
        SOURCES
        "zipf_distribution_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_examples_test
        MODULE random
        SOURCES
        "examples_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        random_seed_sequences_test
        MODULE random
        SOURCES
        "seed_sequences_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_traits_test
        MODULE random
        SOURCES
        "traits_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_generate_real_test
        MODULE random
        SOURCES
        "generate_real_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_library(
        NAMESPACE
        turbo
        NAME
        random_internal_distribution_test_util
        SOURCES
        "chi_square.cc"
        "distribution_test_util.cc"
        HEADERS
        "chi_square.h"
        "distribution_test_util.h"
        CXXOPTS
        ${TURBO_DEFAULT_COPTS}
        ${CARBIN_RANDOM_RANDEN_COPTS}
        ${CARBIN_SIMD_FLAGS}
        LINKS
        ${GTEST_LIB}
        turbo::turbo
)
# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_distribution_test_util_test
        MODULE random
        SOURCES
        "distribution_test_util_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_fastmath_test
        MODULE random
        SOURCES
        "fastmath_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_explicit_seed_seq_test
        MODULE random
        SOURCES
        "explicit_seed_seq_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_salted_seed_seq_test
        MODULE random
        SOURCES
        "salted_seed_seq_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_chi_square_test
        MODULE random
        SOURCES
        "chi_square_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_fast_uniform_bits_test
        MODULE random
        SOURCES
        "fast_uniform_bits_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_nonsecure_base_test
        MODULE random
        SOURCES
        "nonsecure_base_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_seed_material_test
        MODULE random
        SOURCES
        "seed_material_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_pool_urbg_test
        MODULE random
        SOURCES
        "pool_urbg_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_pcg_engine_test
        MODULE random
        SOURCES
        "pcg_engine_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_randen_engine_test
        MODULE random
        SOURCES
        "randen_engine_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_randen_test
        MODULE random
        SOURCES
        "randen_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_randen_slow_test
        MODULE random
        SOURCES
        "randen_slow_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_randen_hwaes_test
        MODULE random
        SOURCES
        "randen_hwaes_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_LIB}
)


# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_uniform_helper_test
        MODULE random
        SOURCES
        "uniform_helper_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_iostream_state_saver_test
        MODULE random
        SOURCES
        "iostream_state_saver_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_test(
        NAME
        random_internal_wide_multiply_test
        MODULE random
        SOURCES
        wide_multiply_test.cc
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)
carbin_cc_test(
        NAME
        exponential_biased_test
        MODULE random
        SOURCES
        "exponential_biased_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)