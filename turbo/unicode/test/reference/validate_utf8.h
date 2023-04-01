// Copyright 2023 The Turbo Authors.
//
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

#include "turbo/unicode/internal/config.h"

namespace turbo {
namespace tests {
namespace reference {

    TURBO_MUST_USE_RESULT bool ValidateUtf8(const char *buf, size_t len) noexcept;

} // namespace reference
} // namespace tests
} // namespace turbo
