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

#pragma once

#include <string>
#include <string_view>
#include <turbo/utility/status.h>
#include <optional>

namespace turbo {

    turbo::Result<std::string> get_env_string(const char *name, std::optional<std::string> default_value = std::nullopt);

    turbo::Result<bool> get_env_bool(const char *name, std::optional<bool> default_value = std::nullopt);

    turbo::Result<int64_t> get_env_int(const char *name, std::optional<int64_t> default_value = std::nullopt);

    turbo::Result<float> get_env_float(const char *name, std::optional<float> default_value = std::nullopt);

    turbo::Result<double> get_env_double(const char *name, std::optional<double> default_value = std::nullopt);

    turbo::Status set_env(const char *name, const char *value, bool overwrite = true);

    turbo::Status set_env_if_not_exist(const char *name, const char *value);

    turbo::Status set_env_bool(const char *name, bool value);

    turbo::Status set_env_bool_if_not_exist(const char *name, bool value);

    turbo::Status set_env_int(const char *name, int64_t value);

    turbo::Status set_env_int_if_not_exist(const char *name, int64_t value);

    turbo::Status set_env_float(const char *name, float value);

    turbo::Status set_env_float_if_not_exist(const char *name, float value);

    turbo::Status set_env_double(const char *name, double value);

    turbo::Status set_env_double_if_not_exist(const char *name, double value);

    turbo::Status unset_env(const char *name);

}  // namespace turbo