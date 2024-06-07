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
#include <turbo/utility/status.h>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace turbo {


    StatusBuilder::StatusBuilder(const turbo::Status &status) : status_(status) {}

    StatusBuilder::StatusBuilder(turbo::Status &&status) : status_(status) {}

    StatusBuilder::StatusBuilder(turbo::StatusCode code) : status_(code, "") {}

    StatusBuilder::StatusBuilder(const StatusBuilder &sb) : status_(sb.status_) {
        if (sb.streamptr_ != nullptr) {
            streamptr_ = std::make_unique<std::ostringstream>(sb.streamptr_->str());
        }
    }

    turbo::Status StatusBuilder::create_status() &&{
        auto result = [&] {
            if (streamptr_->str().empty()) return status_;
            std::string new_msg =
                    turbo::str_cat(status_.message(), "; ", streamptr_->str());
            return turbo::Status(status_.code(), new_msg);
        }();
        status_ = unknown_error("");
        streamptr_ = nullptr;
        return result;
    }

    StatusBuilder &StatusBuilder::log_error() &{ return *this; }

    StatusBuilder &&StatusBuilder::log_error() &&{ return std::move(log_error()); }

    StatusBuilder::operator turbo::Status() const & {
        if (streamptr_ == nullptr) return status_;
        return StatusBuilder(*this).create_status();
    }

    StatusBuilder::operator turbo::Status() && {
        if (streamptr_ == nullptr) return status_;
        return std::move(*this).create_status();
    }

    StatusBuilder aborted_error_builder() { return StatusBuilder(turbo::StatusCode::kAborted); }

    StatusBuilder already_exists_error_builder() {
        return StatusBuilder(turbo::StatusCode::kAlreadyExists);
    }

    StatusBuilder cancelled_error_builder() {
        return StatusBuilder(turbo::StatusCode::kCancelled);
    }

    StatusBuilder failed_precondition_error_builder() {
        return StatusBuilder(turbo::StatusCode::kFailedPrecondition);
    }

    StatusBuilder internal_error_builder() { return StatusBuilder(turbo::StatusCode::kInternal); }

    StatusBuilder invalid_argument_error_builder() {
        return StatusBuilder(turbo::StatusCode::kInvalidArgument);
    }

    StatusBuilder not_found_error_builder() { return StatusBuilder(turbo::StatusCode::kNotFound); }

    StatusBuilder out_of_range_error_builder() {
        return StatusBuilder(turbo::StatusCode::kOutOfRange);
    }

    StatusBuilder unauthenticated_error_builder() {
        return StatusBuilder(turbo::StatusCode::kUnauthenticated);
    }

    StatusBuilder unavailable_error_builder() {
        return StatusBuilder(turbo::StatusCode::kUnavailable);
    }

    StatusBuilder unimplemented_error_builder() {
        return StatusBuilder(turbo::StatusCode::kUnimplemented);
    }

    StatusBuilder unknown_error_builder() { return StatusBuilder(turbo::StatusCode::kUnknown); }

    turbo::Status AnnotateStatus(const turbo::Status& s, std::string_view msg) {
        if (s.ok() || msg.empty()) return s;

        std::string_view new_msg = msg;
        std::string annotated;
        if (!s.message().empty()) {
            turbo::str_append(&annotated, s.message(), "; ", msg);
            new_msg = annotated;
        }
        return turbo::Status(s.code(), new_msg);
    }

    StatusBuilder ret_check_fail(std::string_view msg) {
        return internal_error_builder() << msg;
    }

}  // namespace turbo
