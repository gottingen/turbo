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

#include <memory>
#include <sstream>

#include <turbo/memory/memory.h>
#include <turbo/strings/str_cat.h>
#include <turbo/utility/status_impl.h>
#include <turbo/utility/result_impl.h>

namespace turbo {

    class TURBO_MUST_USE_RESULT StatusBuilder {
    public:
        explicit StatusBuilder(const turbo::Status &status);

        explicit StatusBuilder(turbo::Status &&status);

        explicit StatusBuilder(turbo::StatusCode code);

        StatusBuilder(const StatusBuilder &sb);

        template<typename T>
        StatusBuilder &operator<<(const T &value) &{
            if (status_.ok()) return *this;
            if (streamptr_ == nullptr)
                streamptr_ = std::make_unique<std::ostringstream>();
            *streamptr_ << value;
            return *this;
        }

        template<typename T>
        StatusBuilder &&operator<<(const T &value) &&{
            return std::move(operator<<(value));
        }

        StatusBuilder &log_error() &;

        StatusBuilder &&log_error() &&;

        operator turbo::Status() const &;

        operator turbo::Status() &&;

        template<typename T>
        inline operator turbo::Result<T>() const &{
            if (streamptr_ == nullptr) return turbo::Result<T>(status_);
            return turbo::Result<T>(StatusBuilder(*this).create_status());
        }

        template<typename T>
        inline operator turbo::Result<T>() &&{
            if (streamptr_ == nullptr) return turbo::Result<T>(status_);
            return turbo::Result<T>(StatusBuilder(*this).create_status());
        }

        template<typename Enum>
        StatusBuilder &set_error_code(Enum code) &{
            status_ =
                    turbo::Status(static_cast<turbo::StatusCode>(code), status_.message());
            return *this;
        }

        template<typename Enum>
        StatusBuilder &&set_error_code(Enum code) &&{
            return std::move(set_error_code(code));
        }

        turbo::Status create_status() &&;

    private:
        std::unique_ptr<std::ostringstream> streamptr_;

        turbo::Status status_;
    };

    StatusBuilder aborted_error_builder();

    StatusBuilder already_exists_error_builder();

    StatusBuilder cancelled_error_builder();

    StatusBuilder failed_precondition_error_builder();

    StatusBuilder internal_error_builder();

    StatusBuilder invalid_argument_error_builder();

    StatusBuilder not_found_error_builder();

    StatusBuilder out_of_range_error_builder();

    StatusBuilder unauthenticated_error_builder();

    StatusBuilder unavailable_error_builder();

    StatusBuilder unimplemented_error_builder();

    StatusBuilder unknown_error_builder();

    turbo::Status annotate_status(const turbo::Status &s, std::string_view msg);

    StatusBuilder ret_check_fail(std::string_view msg);
}  // namespace turbo

#ifndef STATUS_MACROS_IMPL
#define STATUS_MACROS_IMPL
#define STATUS_RET_CHECK(cond)         \
  while (TURBO_UNLIKELY(!(cond).ok())) \
  return ret_check_fail(                \
      "STATUS_RET_CHECK "              \
      "failure ")

#define STATUS_RET_CHECK_EQ(lhs, rhs)         \
  while (TURBO_UNLIKELY((lhs) != (rhs))) \
  return ret_check_fail("STATUS_RET_CHECK_EQ failure ")

#define STATUS_RET_CHECK_NE(lhs, rhs)         \
  while (TURBO_UNLIKELY((lhs) == (rhs))) \
  return ret_check_fail("STATUS_RET_CHECK_NE failure ")

#define STATUS_RET_CHECK_GE(lhs, rhs)            \
  while (TURBO_UNLIKELY(!((lhs) >= (rhs)))) \
  return ret_check_fail("STATUS_RET_CHECK_GE failure ")

#define STATUS_RET_CHECK_LE(lhs, rhs)            \
  while (TURBO_UNLIKELY(!((lhs) <= (rhs)))) \
  return ret_check_fail("STATUS_RET_CHECK_LE failure ")

#define STATUS_RET_CHECK_GT(lhs, rhs)           \
  while (TURBO_UNLIKELY(!((lhs) > (rhs)))) \
  return ret_check_fail("STATUS_RET_CHECK_GT failure ")

#define STATUS_RET_CHECK_LT(lhs, rhs)           \
  while (TURBO_UNLIKELY(!((lhs) < (rhs)))) \
  return ret_check_fail("STATUS_RET_CHECK_LT failure ")

#define STATUS_RETURN_IF_ERROR(expr)                      \
  for (auto __return_if_error_res = (expr);              \
       TURBO_UNLIKELY(!__return_if_error_res.ok());) \
  return ::turbo::StatusBuilder(__return_if_error_res)


#define RESULT_STATUS_MACROS_CONCAT_NAME(x, y) RESULT_STATUS_MACROS_CONCAT_IMPL(x, y)
#define RESULT_STATUS_MACROS_CONCAT_IMPL(x, y) x##y

#define RESULT_ASSIGN_OR_RETURN_IMPL(statusor, lhs, rexpr)                \
    auto statusor = (rexpr);                                                \
    if (!statusor.ok()) {                                                   \
           return statusor.status();                                 \
    }                                                                        \
    lhs = std::move(statusor.value());

#define RESULT_ASSIGN_OR_RETURN(lhs, rexpr)                                   \
  RESULT_ASSIGN_OR_RETURN_IMPL(                                                    \
      RESULT_STATUS_MACROS_CONCAT_NAME(_status_or_value, __COUNTER__), lhs, rexpr)
#endif  // STATUS_MACROS_IMPL