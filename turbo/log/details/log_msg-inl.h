// Copyright 2023 The titan-search Authors.
// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include "turbo/log/details/log_msg.h"
#include "turbo/base/sysinfo.h"

namespace turbo::tlog::details {

    log_msg::log_msg(turbo::tlog::log_clock::time_point log_time, turbo::tlog::source_loc loc,
                     string_view_t a_logger_name,
                     turbo::tlog::level::level_enum lvl, turbo::tlog::string_view_t msg)
            : logger_name(a_logger_name), level(lvl), time(log_time)
#ifndef TLOG_NO_THREAD_ID
            , thread_id(turbo::thread_id())
#endif
            , source(loc), payload(msg) {}

    log_msg::log_msg(
            turbo::tlog::source_loc loc, string_view_t a_logger_name, turbo::tlog::level::level_enum lvl,
            turbo::tlog::string_view_t msg)
            : log_msg(os::now(), loc, a_logger_name, lvl, msg) {}

    log_msg::log_msg(string_view_t a_logger_name, turbo::tlog::level::level_enum lvl,
                     turbo::tlog::string_view_t msg)
            : log_msg(os::now(), source_loc{}, a_logger_name, lvl, msg) {}

} // namespace turbo::tlog::details
