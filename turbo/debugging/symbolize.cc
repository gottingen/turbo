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

#include <turbo/debugging/symbolize.h>

#ifdef _WIN32
#include <winapifamily.h>
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)) || \
    WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
// UWP doesn't have access to win32 APIs.
#define TURBO_INTERNAL_HAVE_SYMBOLIZE_WIN32
#endif
#endif

// Emscripten symbolization relies on JS. Do not use them in standalone mode.
#if defined(__EMSCRIPTEN__) && !defined(STANDALONE_WASM)
#define TURBO_INTERNAL_HAVE_SYMBOLIZE_WASM
#endif

#if defined(TURBO_INTERNAL_HAVE_ELF_SYMBOLIZE)
#include <turbo/debugging/symbolize_elf.inc>
#elif defined(TURBO_INTERNAL_HAVE_SYMBOLIZE_WIN32)
// The Windows Symbolizer only works if PDB files containing the debug info
// are available to the program at runtime.
#include <turbo/debugging/symbolize_win32.inc>
#elif defined(__APPLE__)
#include <turbo/debugging/symbolize_darwin.inc>
#elif defined(TURBO_INTERNAL_HAVE_SYMBOLIZE_WASM)
#include <turbo/debugging/symbolize_emscripten.inc>
#else
#include <turbo/debugging/symbolize_unimplemented.inc>
#endif
