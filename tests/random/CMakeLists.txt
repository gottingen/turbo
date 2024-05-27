#
# Copyright (C) 2024 EA group inc.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

carbin_cc_test_library(
        NAMESPACE turbo
        NAME distribution_test_util
        SOURCES distribution_test_util.h distribution_test_util.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test_library(
        NAMESPACE turbo
        NAME chi_square
        SOURCES chi_square.cc chi_square.h
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

set(RANDOM_TEST_SRC
        bernoulli_distribution_test
        beta_distribution_test
        bit_gen_ref_test
        chi_square_test
        discrete_distribution_test
        distributions_test
        distribution_test_util_test
        examples_test
        explicit_seed_seq_test
        exponential_distribution_test
        fastmath_test
        fast_uniform_bits_test
        gaussian_distribution_test
        generate_real_test
        generators_test
        iostream_state_saver_test
        log_uniform_int_distribution_test
        mock_distributions_test
        mocking_bit_gen_test
        nanobenchmark_test
        nonsecure_base_test
        pcg_engine_test
        poisson_distribution_test
        pool_urbg_test
        randen_engine_test
        randen_hwaes_test
        randen_slow_test
        randen_test
        seed_sequences_test
        salted_seed_seq_test
        seed_material_test
        traits_test
        uniform_helper_test
        uniform_int_distribution_test
        uniform_real_distribution_test
        wide_multiply_test
        zipf_distribution_test
)
foreach (test ${RANDOM_TEST_SRC})

    carbin_cc_test(
            NAME ${test}
            MODULE random
            SOURCES ${test}.cc
            LINKS
            turbo::turbo
            chi_square_static
            distribution_test_util_static
            GTest::gtest
            GTest::gmock
            GTest::gmock_main
            ${CARBIN_DEPS_LINK}
            CXXOPTS ${CARBIN_CXX_OPTIONS}
    )
endforeach ()