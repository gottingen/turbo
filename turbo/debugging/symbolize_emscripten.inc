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

#include <cxxabi.h>
#include <emscripten.h>

#include <algorithm>
#include <cstring>

#include <turbo/base/internal/raw_logging.h>
#include <turbo/debugging/internal/demangle.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>

extern "C" {
const char* emscripten_pc_get_function(const void* pc);
}

// clang-format off
EM_JS(bool, HaveOffsetConverter, (),
      { return typeof wasmOffsetConverter !== 'undefined'; });
// clang-format on

namespace turbo {
TURBO_NAMESPACE_BEGIN

void InitializeSymbolizer(const char*) {
  if (!HaveOffsetConverter()) {
    TURBO_RAW_LOG(INFO,
                 "Symbolization unavailable. Rebuild with -sWASM=1 "
                 "and -sUSE_OFFSET_CONVERTER=1.");
  }
}

bool Symbolize(const void* pc, char* out, int out_size) {
  // Check if we have the offset converter necessary for pc_get_function.
  // Without it, the program will abort().
  if (!HaveOffsetConverter()) {
    return false;
  }
  if (pc == nullptr || out_size <= 0) {
    return false;
  }
  const char* func_name = emscripten_pc_get_function(pc);
  if (func_name == nullptr) {
    return false;
  }

  strncpy(out, func_name, out_size);

  if (out[out_size - 1] != '\0') {
    // strncpy() does not '\0' terminate when it truncates.
    static constexpr char kEllipsis[] = "...";
    int ellipsis_size = std::min<int>(sizeof(kEllipsis) - 1, out_size - 1);
    memcpy(out + out_size - ellipsis_size - 1, kEllipsis, ellipsis_size);
    out[out_size - 1] = '\0';
  }

  return true;
}

TURBO_NAMESPACE_END
}  // namespace turbo
