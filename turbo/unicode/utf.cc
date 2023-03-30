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

#include "turbo/unicode/utf.h"
#include "implementation.cc"
#include "encoding_types.cc"
#include "error.cc"
// The large tables should be included once and they
// should not depend on a kernel.
#include "tables/utf8_to_utf16_tables.h"
#include "tables/utf16_to_utf8_tables.h"
// End of tables.

// The scalar routines should be included once.
#include "scalar/ascii.h"
#include "scalar/utf8.h"
#include "scalar/utf16.h"
#include "scalar/utf32.h"

#include "scalar/utf32_to_utf8/valid_utf32_to_utf8.h"
#include "scalar/utf32_to_utf8/utf32_to_utf8.h"

#include "scalar/utf32_to_utf16/valid_utf32_to_utf16.h"
#include "scalar/utf32_to_utf16/utf32_to_utf16.h"

#include "scalar/utf16_to_utf8/valid_utf16_to_utf8.h"
#include "scalar/utf16_to_utf8/utf16_to_utf8.h"

#include "scalar/utf16_to_utf32/valid_utf16_to_utf32.h"
#include "scalar/utf16_to_utf32/utf16_to_utf32.h"

#include "scalar/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "scalar/utf8_to_utf16/utf8_to_utf16.h"

#include "scalar/utf8_to_utf32/valid_utf8_to_utf32.h"
#include "scalar/utf8_to_utf32/utf8_to_utf32.h"
//


SIMDUTF_PUSH_DISABLE_WARNINGS
SIMDUTF_DISABLE_UNDESIRED_WARNINGS


#if SIMDUTF_IMPLEMENTATION_ARM64
#include "arm64/implementation.cc"
#endif
#if SIMDUTF_IMPLEMENTATION_FALLBACK
#include "turbo/unicode/fallback/implementation.cc"
#endif
#if SIMDUTF_IMPLEMENTATION_ICELAKE
#include "icelake/implementation.cc"
#endif
#if SIMDUTF_IMPLEMENTATION_HASWELL
#include "haswell/implementation.cc"
#endif
#if SIMDUTF_IMPLEMENTATION_PPC64
#include "ppc64/implementation.cc"
#endif
#if SIMDUTF_IMPLEMENTATION_WESTMERE
#include "westmere/implementation.cc"
#endif

SIMDUTF_POP_DISABLE_WARNINGS
