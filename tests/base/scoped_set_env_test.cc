// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifdef _WIN32
#include <windows.h>
#endif

#include <gtest/gtest.h>
#include <turbo/base/internal/scoped_set_env.h>

namespace {

    using turbo::base_internal::ScopedSetEnv;

    std::string GetEnvVar(const char *name) {
#ifdef _WIN32
        char buf[1024];
        auto get_res = GetEnvironmentVariableA(name, buf, sizeof(buf));
        if (get_res >= sizeof(buf)) {
          return "TOO_BIG";
        }

        if (get_res == 0) {
          return "UNSET";
        }

        return std::string(buf, get_res);
#else
        const char *val = ::getenv(name);
        if (val == nullptr) {
            return "UNSET";
        }

        return val;
#endif
    }

    TEST(ScopedSetEnvTest, SetNonExistingVarToString) {
        EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");

        {
            ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", "value");

            EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");
        }

        EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");
    }

    TEST(ScopedSetEnvTest, SetNonExistingVarToNull) {
        EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");

        {
            ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", nullptr);

            EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");
        }

        EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");
    }

    TEST(ScopedSetEnvTest, SetExistingVarToString) {
        ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", "value");
        EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");

        {
            ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", "new_value");

            EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "new_value");
        }

        EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");
    }

    TEST(ScopedSetEnvTest, SetExistingVarToNull) {
        ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", "value");
        EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");

        {
            ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", nullptr);

            EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");
        }

        EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");
    }

}  // namespace
