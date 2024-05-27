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


set(SYNC_TEST_SRC
      barrier_test
        blocking_counter_test
        graphcycles_test
        kernel_timeout_test
        lifetime_test
        mutex_method_pointer_test
        mutex_test
        notification_test
        per_thread_sem_test
        waiter_test
)
foreach (test ${SYNC_TEST_SRC})

    carbin_cc_test(
            NAME ${test}
            MODULE synchronization
            SOURCES ${test}.cc
            LINKS
            turbo::turbo
            GTest::gtest
            GTest::gmock
            GTest::gmock_main
            ${CARBIN_DEPS_LINK}
            CXXOPTS ${CARBIN_CXX_OPTIONS}
    )
endforeach ()