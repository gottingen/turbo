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

// See "Retrieving Symbol Information by Address":
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms680578(v=vs.85).aspx

#include <windows.h>

// MSVC header dbghelp.h has a warning for an ignored typedef.
#pragma warning(push)
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)

#pragma comment(lib, "dbghelp.lib")

#include <algorithm>
#include <cstring>

#include <turbo/base/internal/raw_logging.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

static HANDLE process = NULL;

void InitializeSymbolizer(const char*) {
  if (process != nullptr) {
    return;
  }
  process = GetCurrentProcess();

  // Symbols are not loaded until a reference is made requiring the
  // symbols be loaded. This is the fastest, most efficient way to use
  // the symbol handler.
  SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
  if (!SymInitialize(process, nullptr, true)) {
    // GetLastError() returns a Win32 DWORD, but we assign to
    // unsigned long long to simplify the TURBO_RAW_LOG case below.  The uniform
    // initialization guarantees this is not a narrowing conversion.
    const unsigned long long error{GetLastError()};  // NOLINT(runtime/int)
    TURBO_RAW_LOG(FATAL, "SymInitialize() failed: %llu", error);
  }
}

bool Symbolize(const void* pc, char* out, int out_size) {
  if (out_size <= 0) {
    return false;
  }
  alignas(SYMBOL_INFO) char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
  SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buf);
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = MAX_SYM_NAME;
  if (!SymFromAddr(process, reinterpret_cast<DWORD64>(pc), nullptr, symbol)) {
    return false;
  }
  const size_t out_size_t = static_cast<size_t>(out_size);
  strncpy(out, symbol->Name, out_size_t);
  if (out[out_size_t - 1] != '\0') {
    // strncpy() does not '\0' terminate when it truncates.
    static constexpr char kEllipsis[] = "...";
    size_t ellipsis_size =
        std::min(sizeof(kEllipsis) - 1, out_size_t - 1);
    memcpy(out + out_size_t - ellipsis_size - 1, kEllipsis, ellipsis_size);
    out[out_size_t - 1] = '\0';
  }
  return true;
}

TURBO_NAMESPACE_END
}  // namespace turbo
