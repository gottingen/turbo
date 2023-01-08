

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/memory/allocator.h"
#include <cstring>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)

#include <sys/mman.h>
#include <unistd.h>

namespace {
    // This was a static in page_size(), but due to the following TSAN false-positive
    // bug, this has been moved out to a global.
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68338
    const size_t kPageSize = sysconf(_SC_PAGESIZE);

    inline size_t page_size() {
        return kPageSize;
    }

    inline void *allocate_pages(size_t count) {
        auto mapping = mmap(nullptr, count * page_size(), PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        FLARE_CHECK(mapping != MAP_FAILED) << "Failed to allocate" << int(count) << " pages";
        if (mapping == MAP_FAILED) {
            mapping = nullptr;
        }
        return mapping;
    }

    inline void freePages(void *ptr, size_t count) {
        auto res = munmap(ptr, count * page_size());
        (void) res;
        FLARE_CHECK(res == 0) << "Failed to free " << int(count) << " pages at " << ptr;
    }

    inline void protect_page(void *addr) {
        auto res = mprotect(addr, page_size(), PROT_NONE);
        (void) res;
        FLARE_CHECK(res == 0) << "Failed to protect page at " << addr;
    }
}  // anonymous namespace
#elif defined(__Fuchsia__)
#include <unistd.h>
#include <zircon/process.h>
#include <zircon/syscalls.h>
namespace {
// This was a static in page_size(), but due to the following TSAN false-positive
// bug, this has been moved out to a global.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68338
const size_t kPageSize = sysconf(_SC_PAGESIZE);
inline size_t page_size() {
  return kPageSize;
}
inline void* allocate_pages(size_t count) {
  auto length = count * kPageSize;
  zx_handle_t vmo;
  if (zx_vmo_create(length, 0, &vmo) != ZX_OK) {
    return nullptr;
  }
  zx_vaddr_t reservation;
  zx_status_t status =
      zx_vmar_map(zx_vmar_root_self(), ZX_VM_PERM_READ | ZX_VM_PERM_WRITE, 0,
                  vmo, 0, length, &reservation);
  zx_handle_close(vmo);
  (void)status;
  FLARE_CHECK(status == ZX_OK, "Failed to allocate %d pages", int(count));
  return reinterpret_cast<void*>(reservation);
}
inline void freePages(void* ptr, size_t count) {
  auto length = count * kPageSize;
  zx_status_t status = zx_vmar_unmap(zx_vmar_root_self(),
                                     reinterpret_cast<zx_vaddr_t>(ptr), length);
  (void)status;
  FLARE_CHECK(status == ZX_OK, "Failed to free %d pages at %p", int(count),
              ptr);
}
inline void protect_page(void* addr) {
  zx_status_t status = zx_vmar_protect(
      zx_vmar_root_self(), 0, reinterpret_cast<zx_vaddr_t>(addr), kPageSize);
  (void)status;
  FLARE_CHECK(status == ZX_OK, "Failed to protect page at %p", addr);
}
}  // anonymous namespace
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
namespace {
inline size_t page_size() {
  static auto size = [] {
    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    return systemInfo.dwPageSize;
  }();
  return size;
}
inline void* allocate_pages(size_t count) {
  auto mapping = VirtualAlloc(nullptr, count * page_size(),
                              MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  FLARE_CHECK(mapping != nullptr, "Failed to allocate %d pages", int(count));
  return mapping;
}
inline void freePages(void* ptr, size_t count) {
  (void)count;
  auto res = VirtualFree(ptr, 0, MEM_RELEASE);
  (void)res;
  FLARE_CHECK(res != 0, "Failed to free %d pages at %p", int(count), ptr);
}
inline void protect_page(void* addr) {
  DWORD oldVal = 0;
  auto res = VirtualProtect(addr, page_size(), PAGE_NOACCESS, &oldVal);
  (void)res;
  FLARE_CHECK(res != 0, "Failed to protect page at %p", addr);
}
}  // anonymous namespace
#else
#error "Page based allocation not implemented for this platform"
#endif

namespace {

// pagedMalloc() allocates size bytes of uninitialized storage with the
// specified minimum byte alignment using OS specific page mapping calls.
// If guardLow is true then reads or writes to the page below the returned
// address will cause a page fault.
// If guardHigh is true then reads or writes to the page above the allocated
// block will cause a page fault.
// The pointer returned must be freed with pagedFree().
    void *pagedMalloc(size_t alignment,
                      size_t size,
                      bool guardLow,
                      bool guardHigh) {
        (void) alignment;
        FLARE_CHECK(alignment < page_size());
        auto numRequestedPages = (size + page_size() - 1) / page_size();
        auto numTotalPages =
                numRequestedPages + (guardLow ? 1 : 0) + (guardHigh ? 1 : 0);
        auto mem = reinterpret_cast<uint8_t *>(allocate_pages(numTotalPages));
        if (guardLow) {
            protect_page(mem);
            mem += page_size();
        }
        if (guardHigh) {
            protect_page(mem + numRequestedPages * page_size());
        }
        return mem;
    }

    // pagedFree() frees the memory allocated with pagedMalloc().
    void pagedFree(void *ptr,
                   size_t alignment,
                   size_t size,
                   bool guardLow,
                   bool guardHigh) {
        (void) alignment;
        FLARE_CHECK(alignment < page_size());
        auto numRequestedPages = (size + page_size() - 1) / page_size();
        auto numTotalPages =
                numRequestedPages + (guardLow ? 1 : 0) + (guardHigh ? 1 : 0);
        if (guardLow) {
            ptr = reinterpret_cast<uint8_t *>(ptr) - page_size();
        }
        freePages(ptr, numTotalPages);
    }

    // aligned_malloc() allocates size bytes of uninitialized storage with the
    // specified minimum byte alignment. The pointer returned must be freed with
    // alignedFree().
    inline void *aligned_malloc(size_t alignment, size_t size) {
        size_t allocSize = size + alignment + sizeof(void *);
        auto allocation = malloc(allocSize);
        auto aligned = reinterpret_cast<uint8_t *>(flare::base::align_up(
                reinterpret_cast<uintptr_t>(allocation), alignment));  // align
        memcpy(aligned + size, &allocation, sizeof(void *));  // pointer-to-allocation
        return aligned;
    }

    // alignedFree() frees memory allocated by aligned_malloc.
    inline void aligned_free(void *ptr, size_t size) {
        void *base;
        memcpy(&base, reinterpret_cast<uint8_t *>(ptr) + size, sizeof(void *));
        free(base);
    }

    class default_allocator : public flare::allocator {
    public:
        static default_allocator instance;

        virtual flare::allocation allocate(
                const flare::allocation::required_info &request) override {
            void *ptr = nullptr;

            if (request.useGuards) {
                ptr = ::pagedMalloc(request.alignment, request.size, true, true);
            } else if (request.alignment > 1U) {
                ptr = ::aligned_malloc(request.alignment, request.size);
            } else {
                ptr = ::malloc(request.size);
            }

            FLARE_CHECK(ptr != nullptr)<<"allocation failed";
            FLARE_CHECK(reinterpret_cast<uintptr_t>(ptr) % request.alignment == 0)<<"allocation gave incorrect alignment";

            flare::allocation allocation;
            allocation.ptr = ptr;
            allocation.request = request;
            return allocation;
        }

        virtual void free(const flare::allocation &allocation) override {
            if (allocation.request.useGuards) {
                ::pagedFree(allocation.ptr, allocation.request.alignment,
                            allocation.request.size, true, true);
            } else if (allocation.request.alignment > 1U) {
                ::aligned_free(allocation.ptr, allocation.request.size);
            } else {
                ::free(allocation.ptr);
            }
        }
    };

    default_allocator default_allocator::instance;

}  // anonymous namespace

namespace flare {

    allocator *allocator::Default = &default_allocator::instance;

    size_t page_size() {
        return ::page_size();
    }

}  // namespace flare
