//
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
//
// Created by jeff on 24-6-7.
//

#include <turbo/utility/environment.h>
#include <turbo/strings/numbers.h>

namespace turbo {

    turbo::Result<std::string> get_env_string(const char *name, std::optional<std::string> default_value) {
#ifdef _WIN32
        char buf[1024];
        auto get_res = GetEnvironmentVariableA(name, buf, sizeof(buf));
        if (get_res >= sizeof(buf)) {
             if (default_value.has_value()) {
                return *default_value;
            }
            return turbo::resource_exhausted_error("TOO_BIG");
        }

        if (get_res == 0) {
            if (default_value.has_value()) {
                return *default_value;
            }
          return turbo::not_found_error("UNSET");
        }

        return std::string(buf, get_res);
#else
        const char *val = ::getenv(name);
        if (val == nullptr) {
            if (default_value.has_value()) {
                return *default_value;
            }
            return turbo::not_found_error("UNSET");
        }

        return val;
#endif
    }

    turbo::Result<bool> get_env_bool(const char *name, std::optional<bool> default_value) {
        auto rs = get_env_string(name);
        if (!rs.ok()) {
            if (default_value.has_value()) {
                return *default_value;
            }
            return rs.status();
        }
        bool flag;
        if (!turbo::simple_atob(rs.value(), &flag)) {
            return turbo::invalid_argument_error("invalid bool value");
        }
        return flag;
    }

    turbo::Result<int64_t> get_env_int(const char *name, std::optional<int64_t> default_value) {
        auto rs = get_env_string(name);
        if (!rs.ok()) {
            if (default_value.has_value()) {
                return *default_value;
            }
            return rs.status();
        }
        int64_t value;
        if (!turbo::simple_atoi(rs.value(), &value)) {
            return turbo::invalid_argument_error("invalid int value");
        }
        return value;
    }

    turbo::Result<float> get_env_float(const char *name, std::optional<float> default_value) {
        auto rs = get_env_string(name);
        if (!rs.ok()) {
            if (default_value.has_value()) {
                return *default_value;
            }
            return rs.status();
        }
        float value;
        if (!turbo::simple_atof(rs.value(), &value)) {
            return turbo::invalid_argument_error("invalid float value");
        }
        return value;
    }

    turbo::Result<double> get_env_double(const char *name, std::optional<double> default_value) {
        auto rs = get_env_string(name);
        if (!rs.ok()) {
            if (default_value.has_value()) {
                return *default_value;
            }
            return rs.status();
        }
        double value;
        if (!turbo::simple_atod(rs.value(), &value)) {
            return turbo::invalid_argument_error("invalid double value");
        }
        return value;
    }

    turbo::Status set_env_string(const char *name, const char *value, bool overwrite) {
        if (name == nullptr || value == nullptr) {
            return turbo::invalid_argument_error("value is null");
        }

        if (strlen(name) == 0 || strlen(value) == 0) {
            return turbo::invalid_argument_error("name or value is empty");
        }

        if (overwrite) {
            return ::setenv(name, value, 1) == 0 ? turbo::OkStatus() : turbo::errno_to_status(errno, "setenv failed");
        } else {
            return ::setenv(name, value, 0) == 0 ? turbo::OkStatus() : turbo::errno_to_status(errno, "setenv failed");
        }
    }

    turbo::Status set_env_if_not_exist(const char *name, const char *value) {
        return ::setenv(name, value, 0) == 0 ? turbo::OkStatus() : turbo::errno_to_status(errno, "setenv failed");
    }

    turbo::Status set_env_bool(const char *name, bool value) {
        return set_env_string(name, value ? "true" : "false", true);
    }

    turbo::Status set_env_bool_if_not_exist(const char *name, bool value) {
        return set_env_string(name, value ? "true" : "false", false);
    }

    turbo::Status set_env_int(const char *name, int64_t value) {
        return set_env_string(name, std::to_string(value).c_str(), true);
    }

    turbo::Status set_env_int_if_not_exist(const char *name, int64_t value) {
        return set_env_string(name, std::to_string(value).c_str(), false);
    }

    turbo::Status set_env_float(const char *name, float value) {
        return set_env_string(name, std::to_string(value).c_str(), true);
    }

    turbo::Status set_env_float_if_not_exist(const char *name, float value) {
        return set_env_string(name, std::to_string(value).c_str(), false);

    }

    turbo::Status set_env_double(const char *name, double value) {
        return set_env_string(name, std::to_string(value).c_str(), true);
    }

    turbo::Status set_env_double_if_not_exist(const char *name, double value) {
        return set_env_string(name, std::to_string(value).c_str(), false);
    }

    turbo::Status unset_env(const char *name) {
        if (name == nullptr || strlen(name) == 0) {
            return turbo::invalid_argument_error("name is nullptr or empty");
        }
        return ::unsetenv(name) == 0 ? turbo::OkStatus() : turbo::errno_to_status(errno, "unsetenv failed");
    }

}  // namespace turbo
