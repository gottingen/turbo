// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// Created by jeff on 23-12-16.
//


#include <pthread.h>
#include <limits.h>
#include <atomic>
#include "turbo/platform/port.h"
#include "turbo/fiber/internal/fiber_worker.h"
#include "turbo/fiber/internal/key.h"

// Implement fiber_local_key related functions

namespace turbo::fiber_internal {

    class KeyTable;

// defined in task_group.cpp
    extern __thread FiberWorker *tls_task_group;
    extern thread_local fiber_local_storage tls_bls;
    static __thread bool tls_ever_created_keytable = false;

// We keep thread specific data in a two-level array. The top-level array
// contains at most KEY_1STLEVEL_SIZE pointers to dynamically allocated
// arrays of at most KEY_2NDLEVEL_SIZE data pointers. Many applications
// may just occupy one or two second level array, thus this machanism keeps
// memory footprint smaller and we can change KEY_1STLEVEL_SIZE to a
// bigger number more freely. The tradeoff is an additional memory indirection:
// negligible at most time.
    static const uint32_t KEY_2NDLEVEL_SIZE = 32;

// Notice that we're trying to make the memory of second level and first
// level both 256 bytes to make memory allocator happier.
    static const uint32_t KEY_1STLEVEL_SIZE = 31;

// Max tls in one thread, currently the value is 992 which should be enough
// for most projects.
    static const uint32_t KEYS_MAX = KEY_2NDLEVEL_SIZE * KEY_1STLEVEL_SIZE;

// destructors/version of TLS.
    struct KeyInfo {
        uint32_t version;

        void (*dtor)(void *, const void *);

        const void *dtor_args;
    };

    static KeyInfo s_key_info[KEYS_MAX] = {};

    // For allocating keys.
    static std::mutex s_key_mutex;
    static size_t nfreekey = 0;
    static size_t nkey = 0;
    static uint32_t s_free_keys[KEYS_MAX];

    // Stats.
    static std::atomic<size_t> nkeytable;
    static std::atomic<size_t> nsubkeytable;

    // The second-level array.
    // Align with cacheline to avoid false sharing.
    class TURBO_CACHE_LINE_ALIGNED SubKeyTable {
    public:
        SubKeyTable() {
            memset(_data, 0, sizeof(_data));
            nsubkeytable.fetch_add(1, std::memory_order_relaxed);
        }

        // NOTE: Call clear first.
        ~SubKeyTable() {
            nsubkeytable.fetch_sub(1, std::memory_order_relaxed);
        }

        void clear(uint32_t offset) {
            for (uint32_t i = 0; i < KEY_2NDLEVEL_SIZE; ++i) {
                void *p = _data[i].ptr;
                if (p) {
                    // Set the position to nullptr before calling dtor which may set
                    // the position again.
                    _data[i].ptr = nullptr;

                    KeyInfo info = turbo::fiber_internal::s_key_info[offset + i];
                    if (info.dtor && _data[i].version == info.version) {
                        info.dtor(p, info.dtor_args);
                    }
                }
            }
        }

        bool cleared() const {
            // We need to iterate again to check if every slot is empty. An
            // alternative is remember if set_data() was called during clear.
            for (uint32_t i = 0; i < KEY_2NDLEVEL_SIZE; ++i) {
                if (_data[i].ptr) {
                    return false;
                }
            }
            return true;
        }

        inline void *get_data(uint32_t index, uint32_t version) const {
            if (_data[index].version == version) {
                return _data[index].ptr;
            }
            return nullptr;
        }

        inline void set_data(uint32_t index, uint32_t version, void *data) {
            _data[index].version = version;
            _data[index].ptr = data;
        }

    private:
        struct Data {
            uint32_t version;
            void *ptr;
        };
        Data _data[KEY_2NDLEVEL_SIZE];
    };

    // The first-level array.
    // Align with cacheline to avoid false sharing.
    class TURBO_CACHE_LINE_ALIGNED KeyTable {
    public:
        KeyTable() : next(nullptr) {
            memset(_subs, 0, sizeof(_subs));
            nkeytable.fetch_add(1, std::memory_order_relaxed);
        }

        ~KeyTable() {
            nkeytable.fetch_sub(1, std::memory_order_relaxed);
            for (int ntry = 0; ntry < PTHREAD_DESTRUCTOR_ITERATIONS; ++ntry) {
                for (uint32_t i = 0; i < KEY_1STLEVEL_SIZE; ++i) {
                    if (_subs[i]) {
                        _subs[i]->clear(i * KEY_2NDLEVEL_SIZE);
                    }
                }
                bool all_cleared = true;
                for (uint32_t i = 0; i < KEY_1STLEVEL_SIZE; ++i) {
                    if (_subs[i] != nullptr && !_subs[i]->cleared()) {
                        all_cleared = false;
                        break;
                    }
                }
                if (all_cleared) {
                    for (uint32_t i = 0; i < KEY_1STLEVEL_SIZE; ++i) {
                        delete _subs[i];
                    }
                    return;
                }
            }
            TLOG_ERROR("Fail to destroy all objects in KeyTable[{}]", turbo::ptr(this));
        }

        inline void *get_data(fiber_local_key key) const {
            const uint32_t subidx = key.index / KEY_2NDLEVEL_SIZE;
            if (subidx < KEY_1STLEVEL_SIZE) {
                const SubKeyTable *sub_kt = _subs[subidx];
                if (sub_kt) {
                    return sub_kt->get_data(
                            key.index - subidx * KEY_2NDLEVEL_SIZE, key.version);
                }
            }
            return nullptr;
        }

        inline int set_data(fiber_local_key key, void *data) {
            const uint32_t subidx = key.index / KEY_2NDLEVEL_SIZE;
            if (subidx < KEY_1STLEVEL_SIZE &&
                key.version == s_key_info[key.index].version) {
                SubKeyTable *sub_kt = _subs[subidx];
                if (sub_kt == nullptr) {
                    sub_kt = new(std::nothrow) SubKeyTable;
                    if (nullptr == sub_kt) {
                        return ENOMEM;
                    }
                    _subs[subidx] = sub_kt;
                }
                sub_kt->set_data(key.index - subidx * KEY_2NDLEVEL_SIZE,
                                 key.version, data);
                return 0;
            }
            TLOG_CHECK(false, "fiber_setspecific is called on invalid {}", key);
            return EINVAL;
        }

    public:
        KeyTable *next;
    private:
        SubKeyTable *_subs[KEY_1STLEVEL_SIZE];
    };

    static KeyTable *borrow_keytable(fiber_keytable_pool_t *pool) {
        if (pool != nullptr && pool->free_keytables) {
            std::unique_lock l(pool->mutex);
            KeyTable *p = (KeyTable *) pool->free_keytables;
            if (p) {
                pool->free_keytables = p->next;
                return p;
            }
        }
        return nullptr;
    }

    void return_keytable(fiber_keytable_pool_t *pool, KeyTable *kt) {
        if (nullptr == kt) {
            return;
        }
        if (pool == nullptr) {
            delete kt;
            return;
        }
        std::unique_lock mu(pool->mutex);
        if (pool->destroyed) {
            mu.unlock();
            delete kt;
            return;
        }
        kt->next = (KeyTable *) pool->free_keytables;
        pool->free_keytables = kt;
    }

    static void cleanup_pthread(void *arg) {
        KeyTable *kt = static_cast<KeyTable *>(arg);
        if (kt) {
            delete kt;
            // After deletion: tls may be set during deletion.
            tls_bls.keytable = nullptr;
        }
    }

    static void arg_as_dtor(void *data, const void *arg) {
        typedef void (*KeyDtor)(void *);
        return ((KeyDtor) arg)(data);
    }

    static int get_key_count(void *) {
        std::unique_lock l(turbo::fiber_internal::s_key_mutex);
        return (int) nkey - (int) nfreekey;
    }

    static size_t get_keytable_count(void *) {
        return nkeytable.load(std::memory_order_relaxed);
    }

    static size_t get_keytable_memory(void *) {
        const size_t n = nkeytable.load(std::memory_order_relaxed);
        const size_t nsub = nsubkeytable.load(std::memory_order_relaxed);
        return n * sizeof(KeyTable) + nsub * sizeof(SubKeyTable);
    }


    int fiber_keytable_pool_init(fiber_keytable_pool_t *pool) {
        if (pool == nullptr) {
            TLOG_ERROR("Param[pool] is nullptr");
            return EINVAL;
        }
        pool->free_keytables = nullptr;
        pool->destroyed = 0;
        return 0;
    }

    int fiber_keytable_pool_destroy(fiber_keytable_pool_t *pool) {
        if (pool == nullptr) {
            TLOG_ERROR("Param[pool] is nullptr");
            return EINVAL;
        }
        turbo::fiber_internal::KeyTable *saved_free_keytables = nullptr;
        {
            std::unique_lock l(pool->mutex);
            if (pool->free_keytables) {
                saved_free_keytables = (turbo::fiber_internal::KeyTable *) pool->free_keytables;
                pool->free_keytables = nullptr;
            }
            pool->destroyed = 1;
        }
        // Cheat get/setspecific and destroy the keytables.
        turbo::fiber_internal::FiberWorker *const g = turbo::fiber_internal::tls_task_group;
        turbo::fiber_internal::KeyTable *old_kt = turbo::fiber_internal::tls_bls.keytable;
        while (saved_free_keytables) {
            turbo::fiber_internal::KeyTable *kt = saved_free_keytables;
            saved_free_keytables = kt->next;
            turbo::fiber_internal::tls_bls.keytable = kt;
            if (g) {
                g->current_task()->local_storage.keytable = kt;
            }
            delete kt;
            if (old_kt == kt) {
                old_kt = nullptr;
            }
        }
        turbo::fiber_internal::tls_bls.keytable = old_kt;
        if (g) {
            g->current_task()->local_storage.keytable = old_kt;
        }
        // TODO: return_keytable may race with this function, we don't destroy
        // the mutex right now.
        // pthread_mutex_destroy(&pool->mutex);
        return 0;
    }

    int fiber_keytable_pool_getstat(fiber_keytable_pool_t *pool,
                                    fiber_keytable_pool_stat_t *stat) {
        if (pool == nullptr || stat == nullptr) {
            TLOG_ERROR("Param[pool] or Param[stat] is nullptr");
            return EINVAL;
        }
        std::unique_lock mu(pool->mutex);
        size_t count = 0;
        turbo::fiber_internal::KeyTable *p = (turbo::fiber_internal::KeyTable *) pool->free_keytables;
        for (; p; p = p->next, ++count) {}
        stat->nfree = count;
        return 0;
    }

// TODO: this is not strict `reserve' because we only check #free.
// Currently there's no way to track KeyTables that may be returned
// to the pool in future.
    void fiber_keytable_pool_reserve(fiber_keytable_pool_t *pool,
                                     size_t nfree,
                                     fiber_local_key key,
                                     const key_pool_ctor_t &ctor,
                                     const void *ctor_args) {
        if (pool == nullptr) {
            TLOG_ERROR("Param[pool] is nullptr");
            return;
        }
        fiber_keytable_pool_stat_t stat;
        if (fiber_keytable_pool_getstat(pool, &stat) != 0) {
            TLOG_ERROR("Fail to getstat of pool={}", turbo::ptr(pool));
            return;
        }
        for (size_t i = stat.nfree; i < nfree; ++i) {
            turbo::fiber_internal::KeyTable *kt = new(std::nothrow) turbo::fiber_internal::KeyTable;
            if (kt == nullptr) {
                break;
            }
            void *data = ctor(ctor_args);
            if (data) {
                kt->set_data(key, data);
            }  // else append kt w/o data.

            std::unique_lock mu(pool->mutex);
            if (pool->destroyed) {
                mu.unlock();
                delete kt;
                break;
            }
            kt->next = (turbo::fiber_internal::KeyTable *) pool->free_keytables;
            pool->free_keytables = kt;
            if (data == nullptr) {
                break;
            }
        }
    }

    int fiber_key_create2(fiber_local_key *key,
                          void (*dtor)(void *, const void *),
                          const void *dtor_args) {
        uint32_t index = 0;
        {
            std::unique_lock l(turbo::fiber_internal::s_key_mutex);
            if (turbo::fiber_internal::nfreekey > 0) {
                index = turbo::fiber_internal::s_free_keys[--turbo::fiber_internal::nfreekey];
            } else if (turbo::fiber_internal::nkey < turbo::fiber_internal::KEYS_MAX) {
                index = turbo::fiber_internal::nkey++;
            } else {
                return EAGAIN;  // what pthread_key_create returns in this case.
            }
        }
        turbo::fiber_internal::s_key_info[index].dtor = dtor;
        turbo::fiber_internal::s_key_info[index].dtor_args = dtor_args;
        key->index = index;
        key->version = turbo::fiber_internal::s_key_info[index].version;
        if (key->version == 0) {
            ++turbo::fiber_internal::s_key_info[index].version;
            ++key->version;
        }
        return 0;
    }

    int fiber_key_create(fiber_local_key *key, void (*dtor)(void *)) {
        if (dtor == nullptr) {
            return fiber_key_create2(key, nullptr, nullptr);
        } else {
            return fiber_key_create2(key, turbo::fiber_internal::arg_as_dtor, (const void *) dtor);
        }
    }

    int fiber_key_delete(fiber_local_key key) {
        if (key.index < turbo::fiber_internal::KEYS_MAX &&
            key.version == turbo::fiber_internal::s_key_info[key.index].version) {
            std::unique_lock l(turbo::fiber_internal::s_key_mutex);
            if (key.version == turbo::fiber_internal::s_key_info[key.index].version) {
                if (++turbo::fiber_internal::s_key_info[key.index].version == 0) {
                    ++turbo::fiber_internal::s_key_info[key.index].version;
                }
                turbo::fiber_internal::s_key_info[key.index].dtor = nullptr;
                turbo::fiber_internal::s_key_info[key.index].dtor_args = nullptr;
                turbo::fiber_internal::s_free_keys[turbo::fiber_internal::nfreekey++] = key.index;
                return 0;
            }
        }
        TLOG_CHECK(false, "fiber_key_delete is called on invalid {}.{}", key.index, key.version);
        return EINVAL;
    }

    // NOTE: Can't borrow_keytable in fiber_setspecific, otherwise following
    // memory leak may occur:
    //  -> fiber_getspecific fails to borrow_keytable and returns nullptr.
    //  -> fiber_setspecific succeeds to borrow_keytable and overwrites old data
    //     at the position with newly created data, the old data is leaked.
    int fiber_setspecific(fiber_local_key key, void *data) {
        turbo::fiber_internal::KeyTable *kt = turbo::fiber_internal::tls_bls.keytable;
        if (nullptr == kt) {
            kt = new(std::nothrow) turbo::fiber_internal::KeyTable;
            if (nullptr == kt) {
                return ENOMEM;
            }
            turbo::fiber_internal::tls_bls.keytable = kt;
            turbo::fiber_internal::FiberWorker *const g = turbo::fiber_internal::tls_task_group;
            if (g) {
                g->current_task()->local_storage.keytable = kt;
            }
            if (!turbo::fiber_internal::tls_ever_created_keytable) {
                turbo::fiber_internal::tls_ever_created_keytable = true;
                TLOG_CHECK_EQ(0, turbo::thread_atexit(turbo::fiber_internal::cleanup_pthread, kt));
            }
        }
        return kt->set_data(key, data);
    }

    void *fiber_getspecific(fiber_local_key key) {
        turbo::fiber_internal::KeyTable *kt = turbo::fiber_internal::tls_bls.keytable;
        if (kt) {
            return kt->get_data(key);
        }
        turbo::fiber_internal::FiberWorker *const g = turbo::fiber_internal::tls_task_group;
        if (g) {
            turbo::fiber_internal::FiberEntity *const task = g->current_task();
            kt = turbo::fiber_internal::borrow_keytable(task->attr.keytable_pool);
            if (kt) {
                g->current_task()->local_storage.keytable = kt;
                turbo::fiber_internal::tls_bls.keytable = kt;
                return kt->get_data(key);
            }
        }
        return nullptr;
    }

    void fiber_assign_data(void *data) {
        turbo::fiber_internal::tls_bls.assigned_data = data;
    }

    void *fiber_get_assigned_data() {
        return turbo::fiber_internal::tls_bls.assigned_data;
    }

}  // namespace turbo::fiber_internal
