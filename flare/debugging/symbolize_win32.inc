
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

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

#include "flare/log/logging.h"

namespace flare::debugging {


static HANDLE process = nullptr;

void initialize_symbolizer(const char *) {
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
        // unsigned long long to simplify the FLARE_DLOG case below.  The uniform
        // initialization guarantees this is not a narrowing conversion.
        const unsigned long long error{GetLastError()};  // NOLINT(runtime/int)
        FLARE_DLOG(FATAL)<<"SymInitialize() failed: "<< error;
    }
}

bool symbolize(const void *pc, char *out, int out_size) {
    if (out_size <= 0) {
        return false;
    }
    alignas(SYMBOL_INFO) char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO *>(buf);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromAddr(process, reinterpret_cast<DWORD64>(pc), nullptr, symbol)) {
        return false;
    }
    strncpy(out, symbol->Name, out_size);
    if (out[out_size - 1] != '\0') {
        // strncpy() does not '\0' terminate when it truncates.
        static constexpr char kEllipsis[] = "...";
        int ellipsis_size =
                std::min<int>(sizeof(kEllipsis) - 1, out_size - 1);
        memcpy(out + out_size - ellipsis_size - 1, kEllipsis, ellipsis_size);
        out[out_size - 1] = '\0';
    }
    return true;
}


}  // namespace flare::debugging
