
#ifndef FLARE_MEMORY_RESOURCE_POOL_INL_H_
#define FLARE_MEMORY_RESOURCE_POOL_INL_H_

#include <iostream>                      // std::ostream
#include <pthread.h>                     // pthread_mutex_t
#include <algorithm>                     // std::max, std::min
#include <vector>
#include "flare/base/static_atomic.h"              // std::atomic
#include "flare/base/scoped_lock.h"            // FLARE_SCOPED_LOCK
#include "flare/thread/thread.h"           // thread_atexit
#include "flare/base/profile.h"

#ifdef FLARE_RESOURCE_POOL_NEED_FREE_ITEM_NUM
#define FLARE_RESOURCE_POOL_FREE_ITEM_NUM_ADD1                \
    (_global_nfree.fetch_add(1, std::memory_order_relaxed))
#define FLARE_RESOURCE_POOL_FREE_ITEM_NUM_SUB1                \
    (_global_nfree.fetch_sub(1, std::memory_order_relaxed))
#else
#define FLARE_RESOURCE_POOL_FREE_ITEM_NUM_ADD1
#define FLARE_RESOURCE_POOL_FREE_ITEM_NUM_SUB1
#endif

namespace flare {
    
template <typename T>
struct ResourceId {
    uint64_t value;

    operator uint64_t() const {
        return value;
    }

    template <typename T2>
    ResourceId<T2> cast() const {
        ResourceId<T2> id = { value };
        return id;
    }
};

template <typename T, size_t NITEM> 
struct ResourcePoolFreeChunk {
    size_t nfree;
    ResourceId<T> ids[NITEM];
};
// for gcc 3.4.5
template <typename T> 
struct ResourcePoolFreeChunk<T, 0> {
    size_t nfree;
    ResourceId<T> ids[0];
};

struct ResourcePoolInfo {
    size_t local_pool_num;
    size_t block_group_num;
    size_t block_num;
    size_t item_num;
    size_t block_item_num;
    size_t free_chunk_item_num;
    size_t total_size;
#ifdef FLARE_RESOURCE_POOL_NEED_FREE_ITEM_NUM
    size_t free_item_num;
#endif
};

static const size_t RP_MAX_BLOCK_NGROUP = 65536;
static const size_t RP_GROUP_NBLOCK_NBIT = 16;
static const size_t RP_GROUP_NBLOCK = (1UL << RP_GROUP_NBLOCK_NBIT);
static const size_t RP_INITIAL_FREE_LIST_SIZE = 1024;

template <typename T>
class ResourcePoolBlockItemNum {
    static const size_t N1 = ResourcePoolBlockMaxSize<T>::value / sizeof(T);
    static const size_t N2 = (N1 < 1 ? 1 : N1);
public:
    static const size_t value = (N2 > ResourcePoolBlockMaxItem<T>::value ?
                                 ResourcePoolBlockMaxItem<T>::value : N2);
};


template <typename T>
class FLARE_CACHELINE_ALIGNMENT ResourcePool {
public:
    static const size_t BLOCK_NITEM = ResourcePoolBlockItemNum<T>::value;
    static const size_t FREE_CHUNK_NITEM = BLOCK_NITEM;

    // Free identifiers are batched in a FreeChunk before they're added to
    // global list(_free_chunks).
    typedef ResourcePoolFreeChunk<T, FREE_CHUNK_NITEM>      FreeChunk;
    typedef ResourcePoolFreeChunk<T, 0> DynamicFreeChunk;

    // When a thread needs memory, it allocates a Block. To improve locality,
    // items in the Block are only used by the thread.
    // To support cache-aligned objects, align Block.items by cacheline.
    struct FLARE_CACHELINE_ALIGNMENT Block {
        char items[sizeof(T) * BLOCK_NITEM];
        size_t nitem;

        Block() : nitem(0) {}
    };

    // A Resource addresses at most RP_MAX_BLOCK_NGROUP BlockGroups,
    // each BlockGroup addresses at most RP_GROUP_NBLOCK blocks. So a
    // resource addresses at most RP_MAX_BLOCK_NGROUP * RP_GROUP_NBLOCK Blocks.
    struct BlockGroup {
        std::atomic<size_t> nblock;
        std::atomic<Block*> blocks[RP_GROUP_NBLOCK];

        BlockGroup() : nblock(0) {
            // We fetch_add nblock in add_block() before setting the entry,
            // thus address_resource() may sees the unset entry. Initialize
            // all entries to nullptr makes such address_resource() return nullptr.
            memset(static_cast<void*>(blocks), 0, sizeof(std::atomic<Block*>) * RP_GROUP_NBLOCK);
        }
    };


    // Each thread has an instance of this class.
    class FLARE_CACHELINE_ALIGNMENT LocalPool {
    public:
        explicit LocalPool(ResourcePool* pool)
            : _pool(pool)
            , _cur_block(nullptr)
            , _cur_block_index(0) {
            _cur_free.nfree = 0;
        }

        ~LocalPool() {
            // Add to global _free_chunks if there're some free resources
            if (_cur_free.nfree) {
                _pool->push_free_chunk(_cur_free);
            }

            _pool->clear_from_destructor_of_local_pool();
        }

        static void delete_local_pool(void* arg) {
            delete(LocalPool*)arg;
        }

        // We need following macro to construct T with different CTOR_ARGS
        // which may include parenthesis because when T is POD, "new T()"
        // and "new T" are different: former one sets all fields to 0 which
        // we don't want.
#define FLARE_RESOURCE_POOL_GET(CTOR_ARGS)                              \
        /* Fetch local free id */                                       \
        if (_cur_free.nfree) {                                          \
            const ResourceId<T> free_id = _cur_free.ids[--_cur_free.nfree]; \
            *id = free_id;                                              \
            FLARE_RESOURCE_POOL_FREE_ITEM_NUM_SUB1;                   \
            return unsafe_address_resource(free_id);                    \
        }                                                               \
        /* Fetch a FreeChunk from global.                               \
           TODO: Popping from _free needs to copy a FreeChunk which is  \
           costly, but hardly impacts amortized performance. */         \
        if (_pool->pop_free_chunk(_cur_free)) {                         \
            --_cur_free.nfree;                                          \
            const ResourceId<T> free_id =  _cur_free.ids[_cur_free.nfree]; \
            *id = free_id;                                              \
            FLARE_RESOURCE_POOL_FREE_ITEM_NUM_SUB1;                   \
            return unsafe_address_resource(free_id);                    \
        }                                                               \
        /* Fetch memory from local block */                             \
        if (_cur_block && _cur_block->nitem < BLOCK_NITEM) {            \
            id->value = _cur_block_index * BLOCK_NITEM + _cur_block->nitem; \
            T* p = new ((T*)_cur_block->items + _cur_block->nitem) T CTOR_ARGS; \
            if (!ResourcePoolValidator<T>::validate(p)) {               \
                p->~T();                                                \
                return nullptr;                                            \
            }                                                           \
            ++_cur_block->nitem;                                        \
            return p;                                                   \
        }                                                               \
        /* Fetch a Block from global */                                 \
        _cur_block = add_block(&_cur_block_index);                      \
        if (_cur_block != nullptr) {                                       \
            id->value = _cur_block_index * BLOCK_NITEM + _cur_block->nitem; \
            T* p = new ((T*)_cur_block->items + _cur_block->nitem) T CTOR_ARGS; \
            if (!ResourcePoolValidator<T>::validate(p)) {               \
                p->~T();                                                \
                return nullptr;                                            \
            }                                                           \
            ++_cur_block->nitem;                                        \
            return p;                                                   \
        }                                                               \
        return nullptr;                                                    \
 

        inline T* get(ResourceId<T>* id) {
            FLARE_RESOURCE_POOL_GET();
        }

        template <typename A1>
        inline T* get(ResourceId<T>* id, const A1& a1) {
            FLARE_RESOURCE_POOL_GET((a1));
        }

        template <typename A1, typename A2>
        inline T* get(ResourceId<T>* id, const A1& a1, const A2& a2) {
            FLARE_RESOURCE_POOL_GET((a1, a2));
        }

#undef FLARE_RESOURCE_POOL_GET

        inline int return_resource(ResourceId<T> id) {
            // Return to local free list
            if (_cur_free.nfree < ResourcePool::free_chunk_nitem()) {
                _cur_free.ids[_cur_free.nfree++] = id;
                FLARE_RESOURCE_POOL_FREE_ITEM_NUM_ADD1;
                return 0;
            }
            // Local free list is full, return it to global.
            // For copying issue, check comment in upper get()
            if (_pool->push_free_chunk(_cur_free)) {
                _cur_free.nfree = 1;
                _cur_free.ids[0] = id;
                FLARE_RESOURCE_POOL_FREE_ITEM_NUM_ADD1;
                return 0;
            }
            return -1;
        }

    private:
        ResourcePool* _pool;
        Block* _cur_block;
        size_t _cur_block_index;
        FreeChunk _cur_free;
    };

    static inline T* unsafe_address_resource(ResourceId<T> id) {
        const size_t block_index = id.value / BLOCK_NITEM;
        return (T*)(_block_groups[(block_index >> RP_GROUP_NBLOCK_NBIT)]
                    .load(std::memory_order_consume)
                    ->blocks[(block_index & (RP_GROUP_NBLOCK - 1))]
                    .load(std::memory_order_consume)->items) +
               id.value - block_index * BLOCK_NITEM;
    }

    static inline T* address_resource(ResourceId<T> id) {
        const size_t block_index = id.value / BLOCK_NITEM;
        const size_t group_index = (block_index >> RP_GROUP_NBLOCK_NBIT);
        if (__builtin_expect(group_index < RP_MAX_BLOCK_NGROUP, 1)) {
            BlockGroup* bg =
                _block_groups[group_index].load(std::memory_order_consume);
            if (__builtin_expect(bg != nullptr, 1)) {
                Block* b = bg->blocks[block_index & (RP_GROUP_NBLOCK - 1)]
                           .load(std::memory_order_consume);
                if (__builtin_expect(b != nullptr, 1)) {
                    const size_t offset = id.value - block_index * BLOCK_NITEM;
                    if (__builtin_expect(offset < b->nitem, 1)) {
                        return (T*)b->items + offset;
                    }
                }
            }
        }

        return nullptr;
    }

    inline T* get_resource(ResourceId<T>* id) {
        LocalPool* lp = get_or_new_local_pool();
        if (__builtin_expect(lp != nullptr, 1)) {
            return lp->get(id);
        }
        return nullptr;
    }

    template <typename A1>
    inline T* get_resource(ResourceId<T>* id, const A1& arg1) {
        LocalPool* lp = get_or_new_local_pool();
        if (__builtin_expect(lp != nullptr, 1)) {
            return lp->get(id, arg1);
        }
        return nullptr;
    }

    template <typename A1, typename A2>
    inline T* get_resource(ResourceId<T>* id, const A1& arg1, const A2& arg2) {
        LocalPool* lp = get_or_new_local_pool();
        if (__builtin_expect(lp != nullptr, 1)) {
            return lp->get(id, arg1, arg2);
        }
        return nullptr;
    }

    inline int return_resource(ResourceId<T> id) {
        LocalPool* lp = get_or_new_local_pool();
        if (__builtin_expect(lp != nullptr, 1)) {
            return lp->return_resource(id);
        }
        return -1;
    }

    void clear_resources() {
        LocalPool* lp = _local_pool;
        if (lp) {
            _local_pool = nullptr;
            flare::thread::atexit_cancel(LocalPool::delete_local_pool, lp);
            delete lp;
        }
    }

    static inline size_t free_chunk_nitem() {
        const size_t n = ResourcePoolFreeChunkMaxItem<T>::value();
        return n < FREE_CHUNK_NITEM ? n : FREE_CHUNK_NITEM;
    }
    
    // Number of all allocated objects, including being used and free.
    ResourcePoolInfo describe_resources() const {
        ResourcePoolInfo info;
        info.local_pool_num = _nlocal.load(std::memory_order_relaxed);
        info.block_group_num = _ngroup.load(std::memory_order_acquire);
        info.block_num = 0;
        info.item_num = 0;
        info.free_chunk_item_num = free_chunk_nitem();
        info.block_item_num = BLOCK_NITEM;
#ifdef FLARE_RESOURCE_POOL_NEED_FREE_ITEM_NUM
        info.free_item_num = _global_nfree.load(std::memory_order_relaxed);
#endif

        for (size_t i = 0; i < info.block_group_num; ++i) {
            BlockGroup* bg = _block_groups[i].load(std::memory_order_consume);
            if (nullptr == bg) {
                break;
            }
            size_t nblock = std::min(bg->nblock.load(std::memory_order_relaxed),
                                     RP_GROUP_NBLOCK);
            info.block_num += nblock;
            for (size_t j = 0; j < nblock; ++j) {
                Block* b = bg->blocks[j].load(std::memory_order_consume);
                if (nullptr != b) {
                    info.item_num += b->nitem;
                }
            }
        }
        info.total_size = info.block_num * info.block_item_num * sizeof(T);
        return info;
    }

    static inline ResourcePool* singleton() {
        ResourcePool* p = _singleton.load(std::memory_order_consume);
        if (p) {
            return p;
        }
        pthread_mutex_lock(&_singleton_mutex);
        p = _singleton.load(std::memory_order_consume);
        if (!p) {
            p = new ResourcePool();
            _singleton.store(p, std::memory_order_release);
        } 
        pthread_mutex_unlock(&_singleton_mutex);
        return p;
    }

private:
    ResourcePool() {
        _free_chunks.reserve(RP_INITIAL_FREE_LIST_SIZE);
        pthread_mutex_init(&_free_chunks_mutex, nullptr);
    }

    ~ResourcePool() {
        pthread_mutex_destroy(&_free_chunks_mutex);
    }

    // Create a Block and append it to right-most BlockGroup.
    static Block* add_block(size_t* index) {
        Block* const new_block = new(std::nothrow) Block;
        if (nullptr == new_block) {
            return nullptr;
        }

        size_t ngroup;
        do {
            ngroup = _ngroup.load(std::memory_order_acquire);
            if (ngroup >= 1) {
                BlockGroup* const g =
                    _block_groups[ngroup - 1].load(std::memory_order_consume);
                const size_t block_index =
                    g->nblock.fetch_add(1, std::memory_order_relaxed);
                if (block_index < RP_GROUP_NBLOCK) {
                    g->blocks[block_index].store(
                        new_block, std::memory_order_release);
                    *index = (ngroup - 1) * RP_GROUP_NBLOCK + block_index;
                    return new_block;
                }
                g->nblock.fetch_sub(1, std::memory_order_relaxed);
            }
        } while (add_block_group(ngroup));

        // Fail to add_block_group.
        delete new_block;
        return nullptr;
    }

    // Create a BlockGroup and append it to _block_groups.
    // Shall be called infrequently because a BlockGroup is pretty big.
    static bool add_block_group(size_t old_ngroup) {
        BlockGroup* bg = nullptr;
        FLARE_SCOPED_LOCK(_block_group_mutex);
        const size_t ngroup = _ngroup.load(std::memory_order_acquire);
        if (ngroup != old_ngroup) {
            // Other thread got lock and added group before this thread.
            return true;
        }
        if (ngroup < RP_MAX_BLOCK_NGROUP) {
            bg = new(std::nothrow) BlockGroup;
            if (nullptr != bg) {
                // Release fence is paired with consume fence in address() and
                // add_block() to avoid un-constructed bg to be seen by other
                // threads.
                _block_groups[ngroup].store(bg, std::memory_order_release);
                _ngroup.store(ngroup + 1, std::memory_order_release);
            }
        }
        return bg != nullptr;
    }

    inline LocalPool* get_or_new_local_pool() {
        LocalPool* lp = _local_pool;
        if (lp != nullptr) {
            return lp;
        }
        lp = new(std::nothrow) LocalPool(this);
        if (nullptr == lp) {
            return nullptr;
        }
        FLARE_SCOPED_LOCK(_change_thread_mutex); //avoid race with clear()
        _local_pool = lp;
        flare::thread::atexit(LocalPool::delete_local_pool, lp);
        _nlocal.fetch_add(1, std::memory_order_relaxed);
        return lp;
    }

    void clear_from_destructor_of_local_pool() {
        // Remove tls
        _local_pool = nullptr;

        if (_nlocal.fetch_sub(1, std::memory_order_relaxed) != 1) {
            return;
        }

        // Can't delete global even if all threads(called ResourcePool
        // functions) quit because the memory may still be referenced by 
        // other threads. But we need to validate that all memory can
        // be deallocated correctly in tests, so wrap the function with 
        // a macro which is only defined in unittests.
#ifdef FLARE_CLEAR_RESOURCE_POOL_AFTER_ALL_THREADS_QUIT
        FLARE_SCOPED_LOCK(_change_thread_mutex);  // including acquire fence.
        // Do nothing if there're active threads.
        if (_nlocal.load(std::memory_order_relaxed) != 0) {
            return;
        }
        // All threads exited and we're holding _change_thread_mutex to avoid
        // racing with new threads calling get_resource().

        // Clear global free list.
        FreeChunk dummy;
        while (pop_free_chunk(dummy));

        // Delete all memory
        const size_t ngroup = _ngroup.exchange(0, std::memory_order_relaxed);
        for (size_t i = 0; i < ngroup; ++i) {
            BlockGroup* bg = _block_groups[i].load(std::memory_order_relaxed);
            if (nullptr == bg) {
                break;
            }
            size_t nblock = std::min(bg->nblock.load(std::memory_order_relaxed),
                                     RP_GROUP_NBLOCK);
            for (size_t j = 0; j < nblock; ++j) {
                Block* b = bg->blocks[j].load(std::memory_order_relaxed);
                if (nullptr == b) {
                    continue;
                }
                for (size_t k = 0; k < b->nitem; ++k) {
                    T* const objs = (T*)b->items;
                    objs[k].~T();
                }
                delete b;
            }
            delete bg;
        }

        memset(_block_groups, 0, sizeof(BlockGroup*) * RP_MAX_BLOCK_NGROUP);
#endif
    }

private:
    bool pop_free_chunk(FreeChunk& c) {
        // Critical for the case that most return_object are called in
        // different threads of get_object.
        if (_free_chunks.empty()) {
            return false;
        }
        pthread_mutex_lock(&_free_chunks_mutex);
        if (_free_chunks.empty()) {
            pthread_mutex_unlock(&_free_chunks_mutex);
            return false;
        }
        DynamicFreeChunk* p = _free_chunks.back();
        _free_chunks.pop_back();
        pthread_mutex_unlock(&_free_chunks_mutex);
        c.nfree = p->nfree;
        memcpy(c.ids, p->ids, sizeof(*p->ids) * p->nfree);
        free(p);
        return true;
    }

    bool push_free_chunk(const FreeChunk& c) {
        DynamicFreeChunk* p = (DynamicFreeChunk*)malloc(
            offsetof(DynamicFreeChunk, ids) + sizeof(*c.ids) * c.nfree);
        if (!p) {
            return false;
        }
        p->nfree = c.nfree;
        memcpy(p->ids, c.ids, sizeof(*c.ids) * c.nfree);
        pthread_mutex_lock(&_free_chunks_mutex);
        _free_chunks.push_back(p);
        pthread_mutex_unlock(&_free_chunks_mutex);
        return true;
    }
    
    static flare::static_atomic<ResourcePool*> _singleton;
    static pthread_mutex_t _singleton_mutex;
    static FLARE_THREAD_LOCAL LocalPool* _local_pool;
    static flare::static_atomic<long> _nlocal;
    static flare::static_atomic<size_t> _ngroup;
    static pthread_mutex_t _block_group_mutex;
    static pthread_mutex_t _change_thread_mutex;
    static flare::static_atomic<BlockGroup*> _block_groups[RP_MAX_BLOCK_NGROUP];

    std::vector<DynamicFreeChunk*> _free_chunks;
    pthread_mutex_t _free_chunks_mutex;

#ifdef FLARE_RESOURCE_POOL_NEED_FREE_ITEM_NUM
    static flare::static_atomic<size_t> _global_nfree;
#endif
};

// Declare template static variables:

template <typename T>
const size_t ResourcePool<T>::FREE_CHUNK_NITEM;

template <typename T>
FLARE_THREAD_LOCAL typename ResourcePool<T>::LocalPool*
ResourcePool<T>::_local_pool = nullptr;

template <typename T>
flare::static_atomic<ResourcePool<T>*> ResourcePool<T>::_singleton =
    FLARE_STATIC_ATOMIC_INIT(nullptr);

template <typename T>
pthread_mutex_t ResourcePool<T>::_singleton_mutex = PTHREAD_MUTEX_INITIALIZER;

template <typename T>
flare::static_atomic<long> ResourcePool<T>::_nlocal = FLARE_STATIC_ATOMIC_INIT(0);

template <typename T>
flare::static_atomic<size_t> ResourcePool<T>::_ngroup = FLARE_STATIC_ATOMIC_INIT(0);

template <typename T>
pthread_mutex_t ResourcePool<T>::_block_group_mutex = PTHREAD_MUTEX_INITIALIZER;

template <typename T>
pthread_mutex_t ResourcePool<T>::_change_thread_mutex =
    PTHREAD_MUTEX_INITIALIZER;

template <typename T>
flare::static_atomic<typename ResourcePool<T>::BlockGroup*>
ResourcePool<T>::_block_groups[RP_MAX_BLOCK_NGROUP] = {};

#ifdef FLARE_RESOURCE_POOL_NEED_FREE_ITEM_NUM
template <typename T>
flare::static_atomic<size_t> ResourcePool<T>::_global_nfree = FLARE_STATIC_ATOMIC_INIT(0);
#endif

template <typename T>
inline bool operator==(ResourceId<T> id1, ResourceId<T> id2) {
    return id1.value == id2.value;
}

template <typename T>
inline bool operator!=(ResourceId<T> id1, ResourceId<T> id2) {
    return id1.value != id2.value;
}

// Disable comparisons between different typed ResourceId
template <typename T1, typename T2>
bool operator==(ResourceId<T1> id1, ResourceId<T2> id2);

template <typename T1, typename T2>
bool operator!=(ResourceId<T1> id1, ResourceId<T2> id2);

inline std::ostream& operator<<(std::ostream& os,
                                ResourcePoolInfo const& info) {
    return os << "local_pool_num: " << info.local_pool_num
              << "\nblock_group_num: " << info.block_group_num
              << "\nblock_num: " << info.block_num
              << "\nitem_num: " << info.item_num
              << "\nblock_item_num: " << info.block_item_num
              << "\nfree_chunk_item_num: " << info.free_chunk_item_num
              << "\ntotal_size: " << info.total_size;
#ifdef FLARE_RESOURCE_POOL_NEED_FREE_ITEM_NUM
              << "\nfree_num: " << info.free_item_num
#endif
           ;
}

}  // namespace flare

#endif  // FLARE_MEMORY_RESOURCE_POOL_INL_H_
