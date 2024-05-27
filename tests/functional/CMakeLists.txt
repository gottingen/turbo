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

set(FUNCTIONAL_TEST_SRC
        any_invocable_test
        bind_front_test
        function_ref_test
        overload_test
)

foreach (test ${FUNCTIONAL_TEST_SRC})

    carbin_cc_test(
            NAME ${test}
            MODULE functional
            SOURCES ${test}.cc
            DEPS
            test_instance_tracker_static
            LINKS
            turbo::turbo
            test_instance_tracker_static
            GTest::gtest
            GTest::gmock
            GTest::gmock_main
            ${CARBIN_DEPS_LINK}
            CXXOPTS ${CARBIN_CXX_OPTIONS}
    )
endforeach ()