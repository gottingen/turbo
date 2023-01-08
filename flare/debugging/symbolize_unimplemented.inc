
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#include <cstdint>
#include "flare/log/logging.h"

namespace flare::debugging {


namespace debugging_internal {

int InstallSymbolDecorator(SymbolDecorator, void *) { return -1; }

bool RemoveSymbolDecorator(int) { return false; }

bool RemoveAllSymbolDecorators(void) { return false; }

bool RegisterFileMappingHint(const void *, const void *, uint64_t, const char *) {
    return false;
}

bool GetFileMappingHint(const void **, const void **, uint64_t *, const char **) {
    return false;
}

}  // namespace debugging_internal

void initialize_symbolizer(const char *) {}

bool symbolize(const void *, char *, int) { return false; }


}  // namespace flare::debugging
