// Copyright 2020 The Turbo Authors.
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

#include "bad_optional_access.h"

#ifndef TURBO_USES_STD_OPTIONAL

#include <cstdlib>

#include "turbo/base/internal/raw_logging.h"
#include "turbo/platform/port.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

bad_optional_access::~bad_optional_access() = default;

const char* bad_optional_access::what() const noexcept {
  return "optional has no value";
}

namespace optional_internal {

void throw_bad_optional_access() {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw bad_optional_access();
#else
  TURBO_RAW_LOG(FATAL, "Bad optional access");
  abort();
#endif
}

}  // namespace optional_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_USES_STD_OPTIONAL
