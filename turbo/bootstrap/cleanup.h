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
// -----------------------------------------------------------------------------
// File: cleanup.h
// -----------------------------------------------------------------------------
//
// `turbo::Cleanup` implements the scope guard idiom, invoking the contained
// callback's `operator()() &&` on scope exit.
//
// Example:
//
// ```
//   turbo::Status CopyGoodData(const char* source_path, const char* sink_path) {
//     FILE* source_file = fopen(source_path, "r");
//     if (source_file == nullptr) {
//       return turbo::not_found_error("No source file");  // No cleanups execute
//     }
//
//     // C++17 style cleanup using class template argument deduction
//     turbo::Cleanup source_closer = [source_file] { fclose(source_file); };
//
//     FILE* sink_file = fopen(sink_path, "w");
//     if (sink_file == nullptr) {
//       return turbo::not_found_error("No sink file");  // First cleanup executes
//     }
//
//     // C++11 style cleanup using the factory function
//     auto sink_closer = turbo::MakeCleanup([sink_file] { fclose(sink_file); });
//
//     Data data;
//     while (ReadData(source_file, &data)) {
//       if (!data.IsGood()) {
//         turbo::Status result = turbo::failed_precondition_error("Read bad data");
//         return result;  // Both cleanups execute
//       }
//       SaveData(sink_file, &data);
//     }
//
//     return turbo::OkStatus();  // Both cleanups execute
//   }
// ```
//
// Methods:
//
// `std::move(cleanup).Cancel()` will prevent the callback from executing.
//
// `std::move(cleanup).Invoke()` will execute the callback early, before
// destruction, and prevent the callback from executing in the destructor.
//
// Usage:
//
// `turbo::Cleanup` is not an interface type. It is only intended to be used
// within the body of a function. It is not a value type and instead models a
// control flow construct. Check out `defer` in Golang for something similar.

#include <utility>

#include <turbo/base/config.h>
#include <turbo/base/macros.h>
#include <turbo/bootstrap/internal/cleanup.h>

namespace turbo {

    template<typename Arg, typename Callback = void()>
    class TURBO_MUST_USE_RESULT Cleanup final {
        static_assert(cleanup_internal::WasDeduced<Arg>(),
                      "Explicit template parameters are not supported.");

        static_assert(cleanup_internal::ReturnsVoid<Callback>(),
                      "Callbacks that return values are not supported.");

    public:
        Cleanup(Callback callback) : storage_(std::move(callback)) {}  // NOLINT

        Cleanup(Cleanup &&other) = default;

        void Cancel() &&{
            TURBO_HARDENING_ASSERT(storage_.IsCallbackEngaged());
            storage_.DestroyCallback();
        }

        void Invoke() &&{
            TURBO_HARDENING_ASSERT(storage_.IsCallbackEngaged());
            storage_.invoke_callback();
            storage_.DestroyCallback();
        }

        ~Cleanup() {
            if (storage_.IsCallbackEngaged()) {
                storage_.invoke_callback();
                storage_.DestroyCallback();
            }
        }

    private:
        cleanup_internal::Storage<Callback> storage_;
    };

    // `turbo::Cleanup c = /* callback */;`
    //
    // C++17 type deduction API for creating an instance of `turbo::Cleanup`
#if defined(TURBO_HAVE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)
    template<typename Callback>
    Cleanup(Callback callback) -> Cleanup<cleanup_internal::Tag, Callback>;
#endif  // defined(TURBO_HAVE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)

    // `auto c = turbo::MakeCleanup(/* callback */);`
    //
    // C++11 type deduction API for creating an instance of `turbo::Cleanup`
    template<typename... Args, typename Callback>
    turbo::Cleanup<cleanup_internal::Tag, Callback> MakeCleanup(Callback callback) {
        static_assert(cleanup_internal::WasDeduced<cleanup_internal::Tag, Args...>(),
                      "Explicit template parameters are not supported.");

        static_assert(cleanup_internal::ReturnsVoid<Callback>(),
                      "Callbacks that return values are not supported.");

        return {std::move(callback)};
    }
}  // namespace turbo
