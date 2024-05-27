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

// Allow dynamic symbol lookup for in-memory Elf images.

#ifndef TURBO_DEBUGGING_INTERNAL_ELF_MEM_IMAGE_H_
#define TURBO_DEBUGGING_INTERNAL_ELF_MEM_IMAGE_H_

// Including this will define the __GLIBC__ macro if glibc is being
// used.
#include <climits>

#include <turbo/base/config.h>

// Maybe one day we can rewrite this file not to require the elf
// symbol extensions in glibc, but for right now we need them.
#ifdef TURBO_HAVE_ELF_MEM_IMAGE
#error TURBO_HAVE_ELF_MEM_IMAGE cannot be directly set
#endif

#if defined(__ELF__) && !defined(__OpenBSD__) && !defined(__QNX__) && \
    !defined(__native_client__) && !defined(__asmjs__) &&             \
    !defined(__wasm__) && !defined(__HAIKU__) && !defined(__sun) &&   \
    !defined(__VXWORKS__) && !defined(__hexagon__)
#define TURBO_HAVE_ELF_MEM_IMAGE 1
#endif

#ifdef TURBO_HAVE_ELF_MEM_IMAGE

#include <link.h>  // for ElfW

#if defined(__FreeBSD__) && !defined(ElfW)
#define ElfW(x) __ElfN(x)
#endif

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

// An in-memory ELF image (may not exist on disk).
class ElfMemImage {
 private:
  // Sentinel: there could never be an elf image at &kInvalidBaseSentinel.
  static const int kInvalidBaseSentinel;

 public:
  // Sentinel: there could never be an elf image at this address.
  static constexpr const void *const kInvalidBase =
    static_cast<const void*>(&kInvalidBaseSentinel);

  // Information about a single vdso symbol.
  // All pointers are into .dynsym, .dynstr, or .text of the VDSO.
  // Do not free() them or modify through them.
  struct SymbolInfo {
    const char      *name;      // E.g. "__vdso_getcpu"
    const char      *version;   // E.g. "LINUX_2.6", could be ""
                                // for unversioned symbol.
    const void      *address;   // Relocated symbol address.
    const ElfW(Sym) *symbol;    // Symbol in the dynamic symbol table.
  };

  // Supports iteration over all dynamic symbols.
  class SymbolIterator {
   public:
    friend class ElfMemImage;
    const SymbolInfo *operator->() const;
    const SymbolInfo &operator*() const;
    SymbolIterator& operator++();
    bool operator!=(const SymbolIterator &rhs) const;
    bool operator==(const SymbolIterator &rhs) const;
   private:
    SymbolIterator(const void *const image, int index);
    void Update(int incr);
    SymbolInfo info_;
    int index_;
    const void *const image_;
  };


  explicit ElfMemImage(const void *base);
  void                 Init(const void *base);
  bool                 IsPresent() const { return ehdr_ != nullptr; }
  const ElfW(Phdr)*    GetPhdr(int index) const;
  const ElfW(Sym)*     GetDynsym(int index) const;
  const ElfW(Versym)*  GetVersym(int index) const;
  const ElfW(Verdef)*  GetVerdef(int index) const;
  const ElfW(Verdaux)* GetVerdefAux(const ElfW(Verdef) *verdef) const;
  const char*          GetDynstr(ElfW(Word) offset) const;
  const void*          GetSymAddr(const ElfW(Sym) *sym) const;
  const char*          GetVerstr(ElfW(Word) offset) const;
  int                  GetNumSymbols() const;

  SymbolIterator begin() const;
  SymbolIterator end() const;

  // Look up versioned dynamic symbol in the image.
  // Returns false if image is not present, or doesn't contain given
  // symbol/version/type combination.
  // If info_out is non-null, additional details are filled in.
  bool LookupSymbol(const char *name, const char *version,
                    int symbol_type, SymbolInfo *info_out) const;

  // Find info about symbol (if any) which overlaps given address.
  // Returns true if symbol was found; false if image isn't present
  // or doesn't have a symbol overlapping given address.
  // If info_out is non-null, additional details are filled in.
  bool LookupSymbolByAddress(const void *address, SymbolInfo *info_out) const;

 private:
  const ElfW(Ehdr) *ehdr_;
  const ElfW(Sym) *dynsym_;
  const ElfW(Versym) *versym_;
  const ElfW(Verdef) *verdef_;
  const ElfW(Word) *hash_;
  const char *dynstr_;
  size_t strsize_;
  size_t verdefnum_;
  ElfW(Addr) link_base_;     // Link-time base (p_vaddr of first PT_LOAD).
};

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_HAVE_ELF_MEM_IMAGE

#endif  // TURBO_DEBUGGING_INTERNAL_ELF_MEM_IMAGE_H_
