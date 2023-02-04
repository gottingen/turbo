// Copyright 2023 The Turbo Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/log/internal/nullguard.h"

#include <array>

#include "turbo/platform/port.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

TURBO_CONST_INIT const std::array<char, 7> kCharNull{
    {'(', 'n', 'u', 'l', 'l', ')', '\0'}};
TURBO_CONST_INIT const std::array<signed char, 7> kSignedCharNull{
    {'(', 'n', 'u', 'l', 'l', ')', '\0'}};
TURBO_CONST_INIT const std::array<unsigned char, 7> kUnsignedCharNull{
    {'(', 'n', 'u', 'l', 'l', ')', '\0'}};

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
