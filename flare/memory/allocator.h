

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_MEMORY_ALLOCATOR_H_
#define FLARE_MEMORY_ALLOCATOR_H_

#include <cstdint>
#include <array>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <utility>  // std::forward
#include "flare/log/logging.h"

namespace flare {

    template<typename T>
    struct stl_allocator;

    // page_size() returns the size in bytes of a virtual memory page for the host
    // system.
    size_t page_size();

    // aligned_storage() is a replacement for std::aligned_storage that isn't busted
    // on older versions of MSVC.
    template<size_t SIZE, size_t ALIGNMENT>
    struct aligned_storage {
        struct alignas(ALIGNMENT) type {
            unsigned char data[SIZE];
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // allocation
    ///////////////////////////////////////////////////////////////////////////////

    // allocation holds the result of a memory allocation from an Allocator.
    struct allocation {
        // Intended usage of the allocation. Used for allocation trackers.
        enum class Usage : uint8_t {
            Undefined = 0,
            Stack,   // Fiber stack
            Create,  // Allocator::create(), make_unique(), make_shared()
            Vector,  // containers::vector<T>
            List,    // containers::list<T>
            Stl,     // stl_allocator
            Count,   // Not intended to be used as a usage type - used for upper bound.
        };

        // Request holds all the information required to make an allocation.
        struct required_info {
            size_t size = 0;                 // The size of the allocation in bytes.
            size_t alignment = 0;            // The minimum alignment of the allocation.
            bool useGuards = false;          // Whether the allocation is guarded.
            Usage usage = Usage::Undefined;  // Intended usage of the allocation.
        };

        void *ptr = nullptr;  // The pointer to the allocated memory.
        required_info request;      // Request used for the allocation.
    };

    class allocator {
    public:
        // The default allocator. Initialized with an implementation that allocates
        // from the OS. Can be assigned a custom implementation.
        static allocator *Default;

        // inner_deleter is a smart-pointer compatible deleter that can be used to delete
        // objects created by Allocator::create(). inner_deleter is used by the smart
        // pointers returned by make_shared() and make_unique().
        struct inner_deleter {
            inline inner_deleter();

            inline inner_deleter(allocator *allocator, size_t count);

            template<typename T>
            inline void operator()(T *object);

            allocator *alloc = nullptr;
            size_t count = 0;
        };

        // unique_ptr<T> is an alias to std::unique_ptr<T, inner_deleter>.
        template<typename T>
        using unique_ptr = std::unique_ptr<T, inner_deleter>;

        virtual ~allocator() = default;

        // allocate() allocates memory from the allocator.
        // The returned allocation::request field must be equal to the Request
        // parameter.
        virtual allocation allocate(const allocation::required_info &) = 0;

        // free() frees the memory returned by allocate().
        // The allocation must have all fields equal to those returned by allocate().
        virtual void free(const allocation &) = 0;

        // create() allocates and constructs an object of type T, respecting the
        // alignment of the type.
        // The pointer returned by create() must be deleted with destroy().
        template<typename T, typename... ARGS>
        inline T *create(ARGS &&... args);

        // destroy() destructs and frees the object allocated with create().
        template<typename T>
        inline void destroy(T *object);

        // make_unique() returns a new object allocated from the allocator wrapped
        // in a unique_ptr that respects the alignment of the type.
        template<typename T, typename... ARGS>
        inline unique_ptr<T> make_unique(ARGS &&... args);

        // make_unique_n() returns an array of n new objects allocated from the
        // allocator wrapped in a unique_ptr that respects the alignment of the
        // type.
        template<typename T, typename... ARGS>
        inline unique_ptr<T> make_unique_n(size_t n, ARGS &&... args);

        // make_shared() returns a new object allocated from the allocator
        // wrapped in a std::shared_ptr that respects the alignment of the type.
        template<typename T, typename... ARGS>
        inline std::shared_ptr<T> make_shared(ARGS &&... args);

    protected:

        allocator() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Allocator::inner_deleter
    ///////////////////////////////////////////////////////////////////////////////
    allocator::inner_deleter::inner_deleter() : alloc(nullptr) {}

    allocator::inner_deleter::inner_deleter(allocator *alloc, size_t count)
            : alloc(alloc), count(count) {}

    template<typename T>
    void allocator::inner_deleter::operator()(T *object) {
        object->~T();

        allocation allocation;
        allocation.ptr = object;
        allocation.request.size = sizeof(T) * count;
        allocation.request.alignment = alignof(T);
        allocation.request.usage = allocation::Usage::Create;
        alloc->free(allocation);
    }

///////////////////////////////////////////////////////////////////////////////
// Allocator
///////////////////////////////////////////////////////////////////////////////
    template<typename T, typename... ARGS>
    T *allocator::create(ARGS &&... args) {
        allocation::required_info request;
        request.size = sizeof(T);
        request.alignment = alignof(T);
        request.usage = allocation::Usage::Create;

        auto alloc = allocate(request);
        new(alloc.ptr) T(std::forward<ARGS>(args)...);
        return reinterpret_cast<T *>(alloc.ptr);
    }

    template<typename T>
    void allocator::destroy(T *object) {
        object->~T();

        allocation alloc;
        alloc.ptr = object;
        alloc.request.size = sizeof(T);
        alloc.request.alignment = alignof(T);
        alloc.request.usage = allocation::Usage::Create;
        free(alloc);
    }

    template<typename T, typename... ARGS>
    allocator::unique_ptr<T> allocator::make_unique(ARGS &&... args) {
        return make_unique_n<T>(1, std::forward<ARGS>(args)...);
    }

    template<typename T, typename... ARGS>
    allocator::unique_ptr<T> allocator::make_unique_n(size_t n, ARGS &&... args) {
        if (n == 0) {
            return nullptr;
        }

        allocation::required_info request;
        request.size = sizeof(T) * n;
        request.alignment = alignof(T);
        request.usage = allocation::Usage::Create;

        auto alloc = allocate(request);
        new(alloc.ptr) T(std::forward<ARGS>(args)...);
        return unique_ptr<T>(reinterpret_cast<T *>(alloc.ptr), inner_deleter{this, n});
    }

    template<typename T, typename... ARGS>
    std::shared_ptr<T> allocator::make_shared(ARGS &&... args) {
        allocation::required_info request;
        request.size = sizeof(T);
        request.alignment = alignof(T);
        request.usage = allocation::Usage::Create;

        auto alloc = allocate(request);
        new(alloc.ptr) T(std::forward<ARGS>(args)...);
        return std::shared_ptr<T>(reinterpret_cast<T *>(alloc.ptr), inner_deleter{this, 1});
    }

    class tracked_allocator : public allocator {
    public:
        struct UsageStats {
            // Total number of allocations.
            size_t count = 0;
            // total allocation size in bytes (as requested, may be higher due to
            // alignment or guards).
            size_t bytes = 0;
        };

        struct Stats {
            // numAllocations() returns the total number of allocations across all
            // usages for the allocator.
            inline size_t numAllocations() const;

            // bytesAllocated() returns the total number of bytes allocated across all
            // usages for the allocator.
            inline size_t bytesAllocated() const;

            // Statistics per usage.
            std::array<UsageStats, size_t(allocation::Usage::Count)> byUsage;
        };

        // Constructor that wraps an existing allocator.
        inline tracked_allocator(allocator *allocator);

        // stats() returns the current allocator statistics.
        inline Stats stats();

        // Allocator compliance
        inline allocation allocate(const allocation::required_info &) override;

        inline void free(const allocation &) override;

    private:
        allocator *const alloc;
        std::mutex mutex;
        Stats stats_;
    };

    size_t tracked_allocator::Stats::numAllocations() const {
        size_t out = 0;
        for (auto &stats : byUsage) {
            out += stats.count;
        }
        return out;
    }

    size_t tracked_allocator::Stats::bytesAllocated() const {
        size_t out = 0;
        for (auto &stats : byUsage) {
            out += stats.bytes;
        }
        return out;
    }

    tracked_allocator::tracked_allocator(allocator *alloc)
            : alloc(alloc) {}

    tracked_allocator::Stats tracked_allocator::stats() {
        std::unique_lock<std::mutex> lock(mutex);
        return stats_;
    }

    allocation tracked_allocator::allocate(const allocation::required_info &request) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            auto &usageStats = stats_.byUsage[int(request.usage)];
            ++usageStats.count;
            usageStats.bytes += request.size;
        }
        return alloc->allocate(request);
    }

    void tracked_allocator::free(const allocation &allocation) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            auto &usageStats = stats_.byUsage[int(allocation.request.usage)];
            FLARE_CHECK(usageStats.count > 0)<<"tracked_allocator detected abnormal free()";
            FLARE_CHECK(usageStats.bytes >= allocation.request.size)<<"tracked_allocator detected abnormal free()";
            --usageStats.count;
            usageStats.bytes -= allocation.request.size;
        }
        return alloc->free(allocation);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // stl_allocator
    ///////////////////////////////////////////////////////////////////////////////

    // stl_allocator exposes an STL-compatible allocator wrapping a flare::allocator.
    template<typename T>
    struct stl_allocator {
        using value_type = T;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using size_type = size_t;
        using difference_type = size_t;

        // An equivalent STL allocator for a different type.
        template<class U>
        struct rebind {
            typedef stl_allocator<U> other;
        };

        // Constructs an stl_allocator that will allocate using allocator.
        // allocator must remain valid until this stl_allocator has been destroyed.
        inline stl_allocator(allocator *allocator);

        template<typename U>
        inline stl_allocator(const stl_allocator<U> &other);

        // Returns the actual address of x even in presence of overloaded operator&.
        inline pointer address(reference x) const;

        inline const_pointer address(const_reference x) const;

        // Allocates the memory for n objects of type T.
        // Does not actually construct the objects.
        inline T *allocate(std::size_t n);

        // Deallocates the memory for n objects of type T.
        inline void deallocate(T *p, std::size_t n);

        // Returns the maximum theoretically possible number of T stored in this
        // allocator.
        inline size_type max_size() const;

        // Copy constructs an object of type T at the address p.
        inline void construct(pointer p, const_reference val);

        // Constructs an object of type U at the address P forwarning all other
        // arguments to the constructor.
        template<typename U, typename... Args>
        inline void construct(U *p, Args &&... args);

        // Deconstructs the object at p. It does not free the memory.
        inline void destroy(pointer p);

        // Deconstructs the object at p. It does not free the memory.
        template<typename U>
        inline void destroy(U *p);

    private:

        inline allocation::required_info request(size_t n) const;

        template<typename U>
        friend
        struct stl_allocator;
        allocator *_alloc;
    };

    template<typename T>
    stl_allocator<T>::stl_allocator(allocator *alloc) : _alloc(alloc) {}

    template<typename T>
    template<typename U>
    stl_allocator<T>::stl_allocator(const stl_allocator<U> &other) {
        _alloc = other._alloc;
    }

    template<typename T>
    typename stl_allocator<T>::pointer stl_allocator<T>::address(reference x) const {
        return &x;
    }

    template<typename T>
    typename stl_allocator<T>::const_pointer stl_allocator<T>::address(
            const_reference x) const {
        return &x;
    }

    template<typename T>
    T *stl_allocator<T>::allocate(std::size_t n) {
        auto alloc = _alloc->allocate(request(n));
        return reinterpret_cast<T *>(alloc.ptr);
    }

    template<typename T>
    void stl_allocator<T>::deallocate(T *p, std::size_t n) {
        allocation alloc;
        alloc.ptr = p;
        alloc.request = request(n);
        _alloc->free(alloc);
    }

    template<typename T>
    typename stl_allocator<T>::size_type stl_allocator<T>::max_size() const {
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }

    template<typename T>
    void stl_allocator<T>::construct(pointer p, const_reference val) {
        new(p) T(val);
    }

    template<typename T>
    template<typename U, typename... Args>
    void stl_allocator<T>::construct(U *p, Args &&... args) {
        ::new((void *) p) U(std::forward<Args>(args)...);
    }

    template<typename T>
    void stl_allocator<T>::destroy(pointer p) {
        ((T *) p)->~T();
    }

    template<typename T>
    template<typename U>
    void stl_allocator<T>::destroy(U *p) {
        p->~U();
    }

    template<typename T>
    allocation::required_info stl_allocator<T>::request(size_t n) const {
        allocation::required_info req = {};
        req.size = sizeof(T) * n;
        req.alignment = alignof(T);
        req.usage = allocation::Usage::Stl;
        return req;
    }

}  // namespace flare

#endif  // FLARE_MEMORY_ALLOCATOR_H_
