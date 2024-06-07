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
#include <turbo/utility/result_impl.h>

#include <cstdlib>
#include <utility>

#include <turbo/base/call_once.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/nullability.h>
#include <turbo/utility/internal/statusor_internal.h>
#include <turbo/utility/status_impl.h>
#include <turbo/strings/str_cat.h>

namespace turbo {

    BadResultAccess::BadResultAccess(turbo::Status status)
            : status_(std::move(status)) {}

    BadResultAccess::BadResultAccess(const BadResultAccess &other)
            : status_(other.status_) {}

    BadResultAccess &BadResultAccess::operator=(
            const BadResultAccess &other) {
        // Ensure assignment is correct regardless of whether this->InitWhat() has
        // already been called.
        other.InitWhat();
        status_ = other.status_;
        what_ = other.what_;
        return *this;
    }

    BadResultAccess &BadResultAccess::operator=(BadResultAccess &&other) {
        // Ensure assignment is correct regardless of whether this->InitWhat() has
        // already been called.
        other.InitWhat();
        status_ = std::move(other.status_);
        what_ = std::move(other.what_);
        return *this;
    }

    BadResultAccess::BadResultAccess(BadResultAccess &&other)
            : status_(std::move(other.status_)) {}

    turbo::Nonnull<const char *> BadResultAccess::what() const noexcept {
        InitWhat();
        return what_.c_str();
    }

    const turbo::Status &BadResultAccess::status() const { return status_; }

    void BadResultAccess::InitWhat() const {
        turbo::call_once(init_what_, [this] {
            what_ = turbo::str_cat("Bad Result access: ", status_.to_string());
        });
    }

    namespace internal_statusor {

        void Helper::HandleInvalidStatusCtorArg(turbo::Nonnull<turbo::Status *> status) {
            const char *kMessage =
                    "An OK status is not a valid constructor argument to Result<T>";
#ifdef NDEBUG
            TURBO_INTERNAL_LOG(ERROR, kMessage);
#else
            TURBO_INTERNAL_LOG(FATAL, kMessage);
#endif
            // In optimized builds, we will fall back to internal_error.
            *status = turbo::internal_error(kMessage);
        }

        void Helper::Crash(const turbo::Status &status) {
            TURBO_INTERNAL_LOG(
                    FATAL,
                    turbo::str_cat("Attempting to fetch value instead of handling error ",
                                   status.to_string()));
        }

        void ThrowBadStatusOrAccess(turbo::Status status) {
#ifdef TURBO_HAVE_EXCEPTIONS
            throw turbo::BadResultAccess(std::move(status));
#else
            TURBO_INTERNAL_LOG(
                FATAL,
                turbo::str_cat("Attempting to fetch value instead of handling error ",
                             status.ToString()));
            std::abort();
#endif
        }

    }  // namespace internal_statusor
}  // namespace turbo
