#
# Copyright 2022 The Turbo Authors.
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


turbo_cc_test(
        NAME
        random_bit_gen_ref_test
        SRCS
        "bit_gen_ref_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        random_mock_distributions_test
        SRCS
        "mock_distributions_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_mocking_bit_gen_test
        SRCS
        "mocking_bit_gen_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        random_bernoulli_distribution_test
        SRCS
        "bernoulli_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_beta_distribution_test
        SRCS
        "beta_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_distributions_test
        SRCS
        "distributions_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_generators_test
        SRCS
        "generators_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_log_uniform_int_distribution_test
        SRCS
        "log_uniform_int_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_discrete_distribution_test
        SRCS
        "discrete_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_poisson_distribution_test
        SRCS
        "poisson_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_exponential_distribution_test
        SRCS
        "exponential_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_gaussian_distribution_test
        SRCS
        "gaussian_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_uniform_int_distribution_test
        SRCS
        "uniform_int_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_uniform_real_distribution_test
        SRCS
        "uniform_real_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_zipf_distribution_test
        SRCS
        "zipf_distribution_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gmock
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_examples_test
        SRCS
        "examples_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        random_seed_sequences_test
        SRCS
        "seed_sequences_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)


# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_traits_test
        SRCS
        "internal/traits_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_generate_real_test
        SRCS
        "internal/generate_real_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        random_internal_distribution_test_util
        SRCS
        "internal/chi_square.cc"
        "internal/distribution_test_util.cc"
        HDRS
        "internal/chi_square.h"
        "internal/distribution_test_util.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
)
# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_distribution_test_util_test
        SRCS
        "internal/distribution_test_util_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_fastmath_test
        SRCS
        "internal/fastmath_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_explicit_seed_seq_test
        SRCS
        "internal/explicit_seed_seq_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_salted_seed_seq_test
        SRCS
        "internal/salted_seed_seq_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_chi_square_test
        SRCS
        "internal/chi_square_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        turbo::random_internal_distribution_test_util
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_fast_uniform_bits_test
        SRCS
        "internal/fast_uniform_bits_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_nonsecure_base_test
        SRCS
        "internal/nonsecure_base_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_seed_material_test
        SRCS
        "internal/seed_material_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_pool_urbg_test
        SRCS
        "internal/pool_urbg_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_pcg_engine_test
        SRCS
        "internal/pcg_engine_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_randen_engine_test
        SRCS
        "internal/randen_engine_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_randen_test
        SRCS
        "internal/randen_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_randen_slow_test
        SRCS
        "internal/randen_slow_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_randen_hwaes_test
        SRCS
        "internal/randen_hwaes_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest
)


# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_uniform_helper_test
        SRCS
        "internal/uniform_helper_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_iostream_state_saver_test
        SRCS
        "internal/iostream_state_saver_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

# Internal-only target, do not depend on directly.
turbo_cc_test(
        NAME
        random_internal_wide_multiply_test
        SRCS
        internal/wide_multiply_test.cc
        COPTS
        ${TURBO_TEST_COPTS}
        LINKOPTS
        ${TURBO_DEFAULT_LINKOPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)
