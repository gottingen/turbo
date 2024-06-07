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
#include <turbo/flags/usage.h>

#include <stdlib.h>

#include <string>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/const_init.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/flags/internal/usage.h>
#include <turbo/strings/string_view.h>
#include <turbo/synchronization/mutex.h>

namespace turbo {
    namespace flags_internal {
        namespace {
            TURBO_CONST_INIT turbo::Mutex usage_message_guard(turbo::kConstInit);
            TURBO_CONST_INIT std::string *program_usage_message
                    TURBO_GUARDED_BY(usage_message_guard) = nullptr;
        }  // namespace
    }  // namespace flags_internal

    // --------------------------------------------------------------------
    // Sets the "usage" message to be used by help reporting routines.
    void set_program_usage_message(std::string_view new_usage_message) {
        turbo::MutexLock l(&flags_internal::usage_message_guard);

        if (flags_internal::program_usage_message != nullptr) {
            TURBO_INTERNAL_LOG(FATAL, "set_program_usage_message() called twice.");
            std::exit(1);
        }

        flags_internal::program_usage_message = new std::string(new_usage_message);
    }

    // --------------------------------------------------------------------
    // Returns the usage message set by set_program_usage_message().
    // Note: We able to return string_view here only because calling
    // set_program_usage_message twice is prohibited.
    std::string_view program_usage_message() {
        turbo::MutexLock l(&flags_internal::usage_message_guard);

        return flags_internal::program_usage_message != nullptr
               ? std::string_view(*flags_internal::program_usage_message)
               : "Warning: set_program_usage_message() never called";
    }

}  // namespace turbo
