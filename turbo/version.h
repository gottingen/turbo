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

#include "turbo/strings/str_format.h"

// TURBO_VERSION % 100 is the patch level
// TURBO_VERSION / 100 % 1000 is the minor version
// TURBO_VERSION / 100000 is the major version

// current version: 0.6.16
#define TURBO_VERSION 000616

#define TURBO_MAJOR_VERSION TURBO_VERSION / 100000
#define TURBO_MINOR_VERSION TURBO_VERSION / 100 % 1000
#define TURBO_PATCH_VERSION TURBO_VERSION % 100

namespace turbo {

const std::string &version() {
  static const std::string vstr = turbo::StrFormat(
      "%d.%d%d", TURBO_MAJOR_VERSION, TURBO_MINOR_VERSION, TURBO_PATCH_VERSION);
  return vstr;
}

} // namespace turbo

#endif // TURBO_VERSION_H_
