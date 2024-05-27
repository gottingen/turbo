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
        NAME scoped_mock_log
        SOURCES scoped_mock_log.cc scoped_mock_log.h
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test_library(
        NAMESPACE turbo
        NAME test_actions
        SOURCES test_actions.cc test_actions.h
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test_library(
        NAMESPACE turbo
        NAME test_matchers
        SOURCES test_matchers.cc test_matchers.h
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test_library(
        NAMESPACE turbo
        NAME test_helpers
        SOURCES test_helpers.cc test_helpers.h
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

set(LOG_TEST_SRC
        check_test
        die_if_null_test
        flags_test
        fnmatch_test
        globals_test
        log_basic_test
        log_entry_test
        log_format_test
        log_macro_hygiene_test
        log_modifier_methods_test
        log_sink_test
        log_streamer_test
        scoped_mock_log_test
        stderr_log_sink_test
        stripping_test
        structured_test
        turbo_check_test
        turbo_log_basic_test
        vlog_is_on_test
)

foreach (test ${LOG_TEST_SRC})

    carbin_cc_test(
            NAME ${test}
            MODULE log
            SOURCES ${test}.cc
            LINKS
            turbo::turbo
            scoped_mock_log_static
            test_actions_static
            test_matchers_static
            test_helpers_static
            GTest::gtest
            GTest::gmock
            GTest::gmock_main
            ${CARBIN_DEPS_LINK}
            CXXOPTS ${CARBIN_CXX_OPTIONS}
    )
endforeach ()
