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

#include <turbo/base/internal/strerror.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <turbo/base/internal/errno_saver.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {
namespace {

const char* StrErrorAdaptor(int errnum, char* buf, size_t buflen) {
#if defined(_WIN32)
  int rc = strerror_s(buf, buflen, errnum);
  buf[buflen - 1] = '\0';  // guarantee NUL termination
  if (rc == 0 && strncmp(buf, "Unknown error", buflen) == 0) *buf = '\0';
  return buf;
#else
  // The type of `ret` is platform-specific; both of these branches must compile
  // either way but only one will execute on any given platform:
  auto ret = strerror_r(errnum, buf, buflen);
  if (std::is_same<decltype(ret), int>::value) {
    // XSI `strerror_r`; `ret` is `int`:
    if (ret) *buf = '\0';
    return buf;
  } else {
    // GNU `strerror_r`; `ret` is `char *`:
    return reinterpret_cast<const char*>(ret);
  }
#endif
}

std::string StrErrorInternal(int errnum) {
  char buf[100];
  const char* str = StrErrorAdaptor(errnum, buf, sizeof buf);
  if (*str == '\0') {
    snprintf(buf, sizeof buf, "Unknown error %d", errnum);
    str = buf;
  }
  return str;
}

// kSysNerr is the number of errors from a recent glibc. `StrError()` falls back
// to `StrErrorAdaptor()` if the value is larger than this.
constexpr int kSysNerr = 135;

std::array<std::string, kSysNerr>* NewStrErrorTable() {
  auto* table = new std::array<std::string, kSysNerr>;
  for (size_t i = 0; i < table->size(); ++i) {
    (*table)[i] = StrErrorInternal(static_cast<int>(i));
  }
  return table;
}

}  // namespace

std::string StrError(int errnum) {
  turbo::base_internal::ErrnoSaver errno_saver;
  static const auto* table = NewStrErrorTable();
  if (errnum >= 0 && static_cast<size_t>(errnum) < table->size()) {
    return (*table)[static_cast<size_t>(errnum)];
  }
  return StrErrorInternal(errnum);
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo
