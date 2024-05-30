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

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/nullability.h>
#include <turbo/container/inlined_vector.h>
#include <turbo/strings/cord.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/optional.h>

#ifndef SWIG
// Disabled for SWIG as it doesn't parse attributes correctly.
namespace turbo {
// Returned Status objects may not be ignored. Codesearch doesn't handle ifdefs
// as part of a class definitions (b/6995610), so we use a forward declaration.
//
// TODO(b/176172494): TURBO_MUST_USE_RESULT should expand to the more strict
// [[nodiscard]]. For now, just use [[nodiscard]] directly when it is available.
#if TURBO_HAVE_CPP_ATTRIBUTE(nodiscard)
    class [[nodiscard]] TURBO_ATTRIBUTE_TRIVIAL_ABI Status;
#else

    class TURBO_MUST_USE_RESULT TURBO_ATTRIBUTE_TRIVIAL_ABI Status;

#endif
}  // namespace turbo
#endif  // !SWIG

namespace turbo {

    enum class StatusCode : int;
    enum class StatusToStringMode : int;

    namespace status_internal {

        // Container for status payloads.
        struct Payload {
            std::string type_url;
            turbo::Cord payload;
        };

        using Payloads = turbo::InlinedVector<Payload, 1>;

        // Reference-counted representation of Status data.
        class StatusRep {
        public:
            StatusRep(turbo::StatusCode code_arg, turbo::string_view message_arg,
                      std::unique_ptr<status_internal::Payloads> payloads_arg)
                    : ref_(int32_t{1}),
                      code_(code_arg),
                      message_(message_arg),
                      payloads_(std::move(payloads_arg)) {}

            turbo::StatusCode code() const { return code_; }

            const std::string &message() const { return message_; }

            // Ref and unref are const to allow access through a const pointer, and are
            // used during copying operations.
            void Ref() const { ref_.fetch_add(1, std::memory_order_relaxed); }

            void Unref() const;

            // Payload methods correspond to the same methods in turbo::Status.
            turbo::optional<turbo::Cord> get_payload(turbo::string_view type_url) const;

            void set_payload(turbo::string_view type_url, turbo::Cord payload);

            struct EraseResult {
                bool erased;
                uintptr_t new_rep;
            };

            EraseResult erase_payload(turbo::string_view type_url);

            void for_each_payload(
                    turbo::FunctionRef<void(turbo::string_view, const turbo::Cord &)> visitor)
            const;

            std::string ToString(StatusToStringMode mode) const;

            bool operator==(const StatusRep &other) const;

            bool operator!=(const StatusRep &other) const { return !(*this == other); }

            // Returns an equivalent heap allocated StatusRep with refcount 1.
            //
            // `this` is not safe to be used after calling as it may have been deleted.
            turbo::Nonnull<StatusRep *> CloneAndUnref() const;

        private:
            mutable std::atomic<int32_t> ref_;
            turbo::StatusCode code_;

            // As an internal implementation detail, we guarantee that if status.message()
            // is non-empty, then the resulting string_view is null terminated.
            // This is required to implement 'StatusMessageAsCStr(...)'
            std::string message_;
            std::unique_ptr<status_internal::Payloads> payloads_;
        };

        turbo::StatusCode MapToLocalCode(int value);

        // Returns a pointer to a newly-allocated string with the given `prefix`,
        // suitable for output as an error message in assertion/`CHECK()` failures.
        //
        // This is an internal implementation detail for Turbo logging.
        TURBO_ATTRIBUTE_PURE_FUNCTION
        turbo::Nonnull<std::string *> MakeCheckFailString(
                turbo::Nonnull<const turbo::Status *> status,
                turbo::Nonnull<const char *> prefix);

    }  // namespace status_internal

}  // namespace turbo
