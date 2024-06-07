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

#include <turbo/base/internal/scoped_set_env.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdlib>

#include <turbo/base/internal/raw_logging.h>

namespace turbo::base_internal {
    namespace {

#ifdef _WIN32
        const int kMaxEnvVarValueSize = 1024;
#endif

        void SetEnvVar(const char *name, const char *value) {
#ifdef _WIN32
            SetEnvironmentVariableA(name, value);
#else
            if (value == nullptr) {
                ::unsetenv(name);
            } else {
                ::setenv(name, value, 1);
            }
#endif
        }

    }  // namespace

    ScopedSetEnv::ScopedSetEnv(const char *var_name, const char *new_value)
            : var_name_(var_name), was_unset_(false) {
#ifdef _WIN32
        char buf[kMaxEnvVarValueSize];
        auto get_res = GetEnvironmentVariableA(var_name_.c_str(), buf, sizeof(buf));
        TURBO_INTERNAL_CHECK(get_res < sizeof(buf), "value exceeds buffer size");

        if (get_res == 0) {
          was_unset_ = (GetLastError() == ERROR_ENVVAR_NOT_FOUND);
        } else {
          old_value_.assign(buf, get_res);
        }

        SetEnvironmentVariableA(var_name_.c_str(), new_value);
#else
        const char *val = ::getenv(var_name_.c_str());
        if (val == nullptr) {
            was_unset_ = true;
        } else {
            old_value_ = val;
        }
#endif

        SetEnvVar(var_name_.c_str(), new_value);
    }

    ScopedSetEnv::~ScopedSetEnv() {
        SetEnvVar(var_name_.c_str(), was_unset_ ? nullptr : old_value_.c_str());
    }

}  // namespace turbo::base_internal

