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

// This file contains internal parts of the Turbo symbolizer.
// Do not depend on the anything in this file, it may change at anytime.

#ifndef TURBO_DEBUGGING_INTERNAL_SYMBOLIZE_H_
#define TURBO_DEBUGGING_INTERNAL_SYMBOLIZE_H_

#ifdef __cplusplus

#include <cstddef>
#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>

#ifdef TURBO_INTERNAL_HAVE_ELF_SYMBOLIZE
#error TURBO_INTERNAL_HAVE_ELF_SYMBOLIZE cannot be directly set
#elif defined(__ELF__) && defined(__GLIBC__) && !defined(__native_client__) \
      && !defined(__asmjs__) && !defined(__wasm__)
#define TURBO_INTERNAL_HAVE_ELF_SYMBOLIZE 1

#include <elf.h>
#include <link.h>  // For ElfW() macro.
#include <functional>
#include <string>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

// Iterates over all sections, invoking callback on each with the section name
// and the section header.
//
// Returns true on success; otherwise returns false in case of errors.
//
// This is not async-signal-safe.
bool ForEachSection(int fd,
                    const std::function<bool(turbo::string_view name,
                                             const ElfW(Shdr) &)>& callback);

// Gets the section header for the given name, if it exists. Returns true on
// success. Otherwise, returns false.
bool GetSectionHeaderByName(int fd, const char *name, size_t name_len,
                            ElfW(Shdr) *out);

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_ELF_SYMBOLIZE

#ifdef TURBO_INTERNAL_HAVE_DARWIN_SYMBOLIZE
#error TURBO_INTERNAL_HAVE_DARWIN_SYMBOLIZE cannot be directly set
#elif defined(__APPLE__)
#define TURBO_INTERNAL_HAVE_DARWIN_SYMBOLIZE 1
#endif

#ifdef TURBO_INTERNAL_HAVE_EMSCRIPTEN_SYMBOLIZE
#error TURBO_INTERNAL_HAVE_EMSCRIPTEN_SYMBOLIZE cannot be directly set
#elif defined(__EMSCRIPTEN__)
#define TURBO_INTERNAL_HAVE_EMSCRIPTEN_SYMBOLIZE 1
#endif

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

struct SymbolDecoratorArgs {
  // The program counter we are getting symbolic name for.
  const void *pc;
  // 0 for main executable, load address for shared libraries.
  ptrdiff_t relocation;
  // Read-only file descriptor for ELF image covering "pc",
  // or -1 if no such ELF image exists in /proc/self/maps.
  int fd;
  // Output buffer, size.
  // Note: the buffer may not be empty -- default symbolizer may have already
  // produced some output, and earlier decorators may have adorned it in
  // some way. You are free to replace or augment the contents (within the
  // symbol_buf_size limit).
  char *const symbol_buf;
  size_t symbol_buf_size;
  // Temporary scratch space, size.
  // Use that space in preference to allocating your own stack buffer to
  // conserve stack.
  char *const tmp_buf;
  size_t tmp_buf_size;
  // User-provided argument
  void* arg;
};
using SymbolDecorator = void (*)(const SymbolDecoratorArgs *);

// Installs a function-pointer as a decorator. Returns a value less than zero
// if the system cannot install the decorator. Otherwise, returns a unique
// identifier corresponding to the decorator. This identifier can be used to
// uninstall the decorator - See RemoveSymbolDecorator() below.
int InstallSymbolDecorator(SymbolDecorator decorator, void* arg);

// Removes a previously installed function-pointer decorator. Parameter "ticket"
// is the return-value from calling InstallSymbolDecorator().
bool RemoveSymbolDecorator(int ticket);

// Remove all installed decorators.  Returns true if successful, false if
// symbolization is currently in progress.
bool RemoveAllSymbolDecorators();

// Registers an address range to a file mapping.
//
// Preconditions:
//   start <= end
//   filename != nullptr
//
// Returns true if the file was successfully registered.
bool RegisterFileMappingHint(const void* start, const void* end,
                             uint64_t offset, const char* filename);

// Looks up the file mapping registered by RegisterFileMappingHint for an
// address range. If there is one, the file name is stored in *filename and
// *start and *end are modified to reflect the registered mapping. Returns
// whether any hint was found.
bool GetFileMappingHint(const void** start, const void** end, uint64_t* offset,
                        const char** filename);

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // __cplusplus

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
#endif  // __cplusplus

    bool
    TurboInternalGetFileMappingHint(const void** start, const void** end,
                                   uint64_t* offset, const char** filename);

#endif  // TURBO_DEBUGGING_INTERNAL_SYMBOLIZE_H_
