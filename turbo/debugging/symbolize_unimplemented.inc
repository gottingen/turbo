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

#include <cstdint>

#include <turbo/base/internal/raw_logging.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace debugging_internal {

int InstallSymbolDecorator(SymbolDecorator, void*) { return -1; }
bool RemoveSymbolDecorator(int) { return false; }
bool RemoveAllSymbolDecorators(void) { return false; }
bool RegisterFileMappingHint(const void *, const void *, uint64_t, const char *) {
  return false;
}
bool GetFileMappingHint(const void **, const void **, uint64_t *, const char **) {
  return false;
}

}  // namespace debugging_internal

void InitializeSymbolizer(const char*) {}
bool Symbolize(const void *, char *, int) { return false; }

TURBO_NAMESPACE_END
}  // namespace turbo
