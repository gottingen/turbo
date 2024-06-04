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

#include <new>
#include <type_traits>
#include <utility>

#include <turbo/base/internal/invoke.h>
#include <turbo/base/macros.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/utility/utility.h>

namespace turbo {

    namespace cleanup_internal {

        struct Tag {
        };

        template<typename Arg, typename... Args>
        constexpr bool WasDeduced() {
            return (std::is_same<cleanup_internal::Tag, Arg>::value) &&
                   (sizeof...(Args) == 0);
        }

        template<typename Callback>
        constexpr bool ReturnsVoid() {
            return (std::is_same<base_internal::invoke_result_t<Callback>, void>::value);
        }

        template<typename Callback>
        class Storage {
        public:
            Storage() = delete;

            explicit Storage(Callback callback) {
                // Placement-new into a character buffer is used for eager destruction when
                // the cleanup is invoked or cancelled. To ensure this optimizes well, the
                // behavior is implemented locally instead of using an turbo::optional.
                ::new(GetCallbackBuffer()) Callback(std::move(callback));
                is_callback_engaged_ = true;
            }

            Storage(Storage &&other) {
                TURBO_HARDENING_ASSERT(other.IsCallbackEngaged());

                ::new(GetCallbackBuffer()) Callback(std::move(other.GetCallback()));
                is_callback_engaged_ = true;

                other.DestroyCallback();
            }

            Storage(const Storage &other) = delete;

            Storage &operator=(Storage &&other) = delete;

            Storage &operator=(const Storage &other) = delete;

            void *GetCallbackBuffer() { return static_cast<void *>(+callback_buffer_); }

            Callback &GetCallback() {
                return *reinterpret_cast<Callback *>(GetCallbackBuffer());
            }

            bool IsCallbackEngaged() const { return is_callback_engaged_; }

            void DestroyCallback() {
                is_callback_engaged_ = false;
                GetCallback().~Callback();
            }

            void InvokeCallback() TURBO_NO_THREAD_SAFETY_ANALYSIS {
                std::move(GetCallback())();
            }

        private:
            bool is_callback_engaged_;
            alignas(Callback) char callback_buffer_[sizeof(Callback)];
        };

    }  // namespace cleanup_internal

}  // namespace turbo
