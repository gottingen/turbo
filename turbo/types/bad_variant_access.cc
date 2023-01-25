// Copyright 2017 The Turbo Authors.
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

#include "turbo/types/bad_variant_access.h"

#ifndef TURBO_USES_STD_VARIANT

#include <cstdlib>
#include <stdexcept>

#include "turbo/platform/config.h"
#include "turbo/platform/internal/raw_logging.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

//////////////////////////
// [variant.bad.access] //
//////////////////////////

bad_variant_access::~bad_variant_access() = default;

const char* bad_variant_access::what() const noexcept {
  return "Bad variant access";
}

namespace variant_internal {

void ThrowBadVariantAccess() {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw bad_variant_access();
#else
  TURBO_RAW_LOG(FATAL, "Bad variant access");
  abort();  // TODO(calabrese) Remove once RAW_LOG FATAL is noreturn.
#endif
}

void Rethrow() {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw;
#else
  TURBO_RAW_LOG(FATAL,
               "Internal error in turbo::variant implementation. Attempted to "
               "rethrow an exception when building with exceptions disabled.");
  abort();  // TODO(calabrese) Remove once RAW_LOG FATAL is noreturn.
#endif
}

}  // namespace variant_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_USES_STD_VARIANT
