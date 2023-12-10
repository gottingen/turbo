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

#ifndef TURBO_VERSION_H_
#define TURBO_VERSION_H_

#include "turbo/format/format.h"
#include "turbo/strings/version.h"
#include "turbo/unicode/version.h"

namespace turbo {

    /**
     * @brief Turbo version this is the version of the whole library
     *        it should be updated when any of the submodules are updated.
     *        The version is in the format of major.minor.patch
     *        major: major changes, API changes, breaking changes
     *        minor: minor changes, new features, new modules
     *        patch: bug fixes, performance improvements
     *        The version is used to check if the library is compatible with the
     *        application.
     */
    static constexpr turbo::ModuleVersion version = turbo::ModuleVersion{0, 9, 36};

} // namespace turbo

#endif // TURBO_VERSION_H_
