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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"
#include <algorithm>                         // std::sort
#include <atomic>
#include "turbo/times/stop_watcher.h"
#include "turbo/log/logging.h"
#include "turbo/fiber/fiber.h"
#include "turbo/fiber/fiber_local.h"



namespace turbo::fiber_internal {

    int fiber_keytable_pool_size(fiber_keytable_pool_t *pool) {
        fiber_keytable_pool_stat_t s;
        if (fiber_keytable_pool_getstat(pool, &s) != 0) {
            return 0;
        }
        return s.nfree;
    }


    // Count tls usages.
    struct Counters {
        std::atomic<size_t> ncreate;
        std::atomic<size_t> ndestroy;
        std::atomic<size_t> nenterthread;
        std::atomic<size_t> nleavethread;
    };

    // Wrap same counters into different objects to make sure that different key
    // returns different objects as well as aggregate the usages.
    struct CountersWrapper {
        CountersWrapper(Counters *c, fiber_local_key key) : _c(c), _key(key) {}

        ~CountersWrapper() {
            if (_c) {
                _c->ndestroy.fetch_add(1, std::memory_order_relaxed);
            }
            TLOG_CHECK_EQ(0, fiber_key_delete(_key));
        }

    private:
        Counters *_c;
        fiber_local_key _key;
    };

    static void destroy_counters_wrapper(void *arg) {
        delete static_cast<CountersWrapper *>(arg);
    }

    const size_t NKEY_PER_WORKER = 32;

// NOTE: returns void to use ASSERT
    static void worker1_impl(Counters *cs) {
        cs->nenterthread.fetch_add(1, std::memory_order_relaxed);
        fiber_local_key k[NKEY_PER_WORKER];
        CountersWrapper *ws[TURBO_ARRAY_SIZE(k)];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(k); ++i) {
            REQUIRE_EQ(0, fiber_key_create(&k[i], destroy_counters_wrapper));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(k); ++i) {
            ws[i] = new CountersWrapper(cs, k[i]);
        }
        // Get just-created tls should return nullptr.
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(k); ++i) {
            REQUIRE_EQ(nullptr, fiber_getspecific(k[i]));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(k); ++i) {
            cs->ncreate.fetch_add(1, std::memory_order_relaxed);
            REQUIRE_EQ(0, fiber_setspecific(k[i], ws[i]));

        }
        // Sleep a while to make some context switches. TLS should be unchanged.
        turbo::fiber_sleep_for(turbo::Duration::microseconds(10000));

        for (size_t i = 0; i < TURBO_ARRAY_SIZE(k); ++i) {
            REQUIRE_EQ(ws[i], fiber_getspecific(k[i]));
        }
        cs->nleavethread.fetch_add(1, std::memory_order_relaxed);
    }

    static void *worker1(void *arg) {
        worker1_impl(static_cast<Counters *>(arg));
        return nullptr;
    }

    TEST_CASE("KeyTest, creating_key_in_parallel") {
        Counters args;
        memset(&args, 0, sizeof(args));
        pthread_t th[8];
        fiber_id_t bth[8];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, worker1, &args));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(bth); ++i) {
            REQUIRE(fiber_start_background(&bth[i], nullptr, worker1, &args).ok());
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_join(th[i], nullptr));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(bth); ++i) {
            REQUIRE(fiber_join(bth[i], nullptr).ok());
        }
        REQUIRE_EQ(TURBO_ARRAY_SIZE(th) + TURBO_ARRAY_SIZE(bth),
                   args.nenterthread.load(std::memory_order_relaxed));
        REQUIRE_EQ(TURBO_ARRAY_SIZE(th) + TURBO_ARRAY_SIZE(bth),
                   args.nleavethread.load(std::memory_order_relaxed));
        REQUIRE_EQ(NKEY_PER_WORKER * (TURBO_ARRAY_SIZE(th) + TURBO_ARRAY_SIZE(bth)),
                   args.ncreate.load(std::memory_order_relaxed));
        REQUIRE_EQ(NKEY_PER_WORKER * (TURBO_ARRAY_SIZE(th) + TURBO_ARRAY_SIZE(bth)),
                   args.ndestroy.load(std::memory_order_relaxed));
    }

    std::atomic<size_t> seq(1);
    std::vector<size_t> seqs;
    std::mutex mutex;

    void dtor2(void *arg) {
        std::unique_lock l(mutex);
        seqs.push_back((size_t) arg);
    }

// NOTE: returns void to use ASSERT
    static void worker2_impl(fiber_local_key k) {
        REQUIRE_EQ(nullptr, fiber_getspecific(k));
        REQUIRE_EQ(0, fiber_setspecific(k, (void *) seq.fetch_add(1)));
    }

    static void *worker2(void *arg) {
        worker2_impl(*static_cast<fiber_local_key *>(arg));
        return nullptr;
    }

    TEST_CASE("KeyTest, use_one_key_in_different_threads") {
        fiber_local_key k;
        REQUIRE_EQ(0, fiber_key_create(&k, dtor2));
        seqs.clear();

        pthread_t th[16];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, worker2, &k));
        }
        fiber_id_t bth[1];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(bth); ++i) {
            REQUIRE(fiber_start_urgent(&bth[i], nullptr, worker2, &k).ok());
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_join(th[i], nullptr));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(bth); ++i) {
            REQUIRE(fiber_join(bth[i], nullptr).ok());
        }
        REQUIRE_EQ(TURBO_ARRAY_SIZE(th) + TURBO_ARRAY_SIZE(bth), seqs.size());
        std::sort(seqs.begin(), seqs.end());
        REQUIRE_EQ(seqs.end(), std::unique(seqs.begin(), seqs.end()));
        REQUIRE_EQ(TURBO_ARRAY_SIZE(th) + TURBO_ARRAY_SIZE(bth) - 1, *(seqs.end() - 1) - *seqs.begin());

        REQUIRE_EQ(0, fiber_key_delete(k));
    }

    struct Keys {
        fiber_local_key valid_key;
        fiber_local_key invalid_key;
    };

    void *const DUMMY_PTR = (void *) 1;

    void use_invalid_keys_impl(const Keys *keys) {
        REQUIRE_EQ(nullptr, fiber_getspecific(keys->invalid_key));
        // valid key returns nullptr as well.
        REQUIRE_EQ(nullptr, fiber_getspecific(keys->valid_key));

        // both pthread_setspecific(of nptl) and fiber_setspecific should find
        // the key is invalid.
        REQUIRE_EQ(EINVAL, fiber_setspecific(keys->invalid_key, DUMMY_PTR));
        REQUIRE_EQ(0, fiber_setspecific(keys->valid_key, DUMMY_PTR));

        // Print error again.
        REQUIRE_EQ(nullptr, fiber_getspecific(keys->invalid_key));
        REQUIRE_EQ(DUMMY_PTR, fiber_getspecific(keys->valid_key));
    }

    void *use_invalid_keys(void *args) {
        use_invalid_keys_impl(static_cast<const Keys *>(args));
        return nullptr;
    }

    TEST_CASE("KeyTest, use_invalid_keys") {
        Keys keys;
        REQUIRE_EQ(0, fiber_key_create(&keys.valid_key, nullptr));
// intended to be a created but invalid key.
        keys.invalid_key.index = keys.valid_key.index;
        keys.invalid_key.version = 123;

        pthread_t th;
        fiber_id_t bth;
        REQUIRE_EQ(0, pthread_create(&th, nullptr, use_invalid_keys, &keys));
        REQUIRE(fiber_start_urgent(&bth, nullptr, use_invalid_keys, &keys).ok());
        REQUIRE_EQ(0, pthread_join(th, nullptr));
        REQUIRE(fiber_join(bth, nullptr).ok());
        REQUIRE_EQ(0, fiber_key_delete(keys.valid_key));
    }

    TEST_CASE("KeyTest, reuse_key") {
        fiber_local_key key;
        REQUIRE_EQ(0, fiber_key_create(&key, nullptr));
        REQUIRE_EQ(nullptr, fiber_getspecific(key));
        REQUIRE_EQ(0, fiber_setspecific(key, (void *) 1));
        REQUIRE_EQ(0, fiber_key_delete(key)); // delete key before clearing TLS.
        fiber_local_key key2;
        REQUIRE_EQ(0, fiber_key_create(&key2, nullptr));
        REQUIRE_EQ(key.index, key2.index);
        // The slot is not nullptr, the impl must check version and return nullptr.
        REQUIRE_EQ(nullptr, fiber_getspecific(key2));
    }

    // NOTE: sid is short for 'set in dtor'.
    struct SidData {
        fiber_local_key key;
        int seq;
        int end_seq;
    };

    static void sid_dtor(void *tls) {
        SidData *data = (SidData *) tls;
        // Should already be set nullptr.
        REQUIRE_EQ(nullptr, fiber_getspecific(data->key));
        if (++data->seq < data->end_seq) {
            REQUIRE_EQ(0, fiber_setspecific(data->key, data));
        }
    }

    static void sid_thread_impl(SidData *data) {
        REQUIRE_EQ(0, fiber_setspecific(data->key, data));
    };

    static void *sid_thread(void *args) {
        sid_thread_impl((SidData *) args);
        return nullptr;
    }

    TEST_CASE("KeyTest, set_in_dtor") {
        fiber_local_key key;
        REQUIRE_EQ(0, fiber_key_create(&key, sid_dtor));

        SidData pth_data = {key, 0, 3};
        SidData bth_data = {key, 0, 3};
        SidData fib2_data = {key, 0, 3};

        pthread_t pth;
        fiber_id_t bth;
        fiber_id_t bth2;
        REQUIRE_EQ(0, pthread_create(&pth, nullptr, sid_thread, &pth_data));
        REQUIRE(fiber_start_urgent(&bth, nullptr, sid_thread, &bth_data).ok());
        REQUIRE(fiber_start_urgent(&bth2, &FIBER_ATTR_PTHREAD,
                                   sid_thread, &fib2_data).ok());

        REQUIRE_EQ(0, pthread_join(pth, nullptr));
        REQUIRE(fiber_join(bth, nullptr).ok());
        REQUIRE(fiber_join(bth2, nullptr).ok());

        REQUIRE_EQ(0, fiber_key_delete(key));

        REQUIRE_EQ(pth_data.end_seq, pth_data.seq);
        REQUIRE_EQ(bth_data.end_seq, bth_data.seq);
        REQUIRE_EQ(fib2_data.end_seq, fib2_data.seq);
    }

    struct SBAData {
        fiber_local_key key;
        int level;
        int ndestroy;
    };

    struct SBATLS {
        int *ndestroy;

        static void deleter(void *d) {
            SBATLS *tls = (SBATLS *) d;
            ++*tls->ndestroy;
            delete tls;
        }
    };

    void *set_before_any_fiber(void *args);

    void set_before_any_fiber_impl(SBAData *data) {
        REQUIRE_EQ(nullptr, fiber_getspecific(data->key));
        SBATLS *tls = new SBATLS;
        tls->ndestroy = &data->ndestroy;
        REQUIRE_EQ(0, fiber_setspecific(data->key, tls));
        REQUIRE_EQ(tls, fiber_getspecific(data->key));
        if (data->level++ == 0) {
            fiber_id_t bth;
            REQUIRE(fiber_start_urgent(&bth, nullptr, set_before_any_fiber, data).ok());
            REQUIRE(fiber_join(bth, nullptr).ok());
            REQUIRE_EQ(1, data->ndestroy);
        } else {
            turbo::fiber_sleep_for(turbo::Duration::microseconds(1000));
        }
        REQUIRE_EQ(tls, fiber_getspecific(data->key));
    }

    void *set_before_any_fiber(void *args) {
        set_before_any_fiber_impl((SBAData *) args);
        return nullptr;
    }

    TEST_CASE("KeyTest, set_tls_before_creating_any_fiber") {
        fiber_local_key key;
        REQUIRE_EQ(0, fiber_key_create(&key, SBATLS::deleter));
        pthread_t th;
        SBAData data;
        data.key = key;
        data.level = 0;
        data.ndestroy = 0;
        REQUIRE_EQ(0, pthread_create(&th, nullptr, set_before_any_fiber, &data));
        REQUIRE_EQ(0, pthread_join(th, nullptr));
        REQUIRE_EQ(0, fiber_key_delete(key));
        REQUIRE_EQ(2, data.level);
        REQUIRE_EQ(2, data.ndestroy);
    }

    struct PoolData {
        fiber_local_key key;
        PoolData *expected_data;
        int seq;
        int end_seq;
    };

    static void pool_thread_impl(PoolData *data) {
        REQUIRE_EQ(data->expected_data, (PoolData *) fiber_getspecific(data->key));
        if (nullptr == fiber_getspecific(data->key)) {
            REQUIRE_EQ(0, fiber_setspecific(data->key, data));
        }
    };

    static void *pool_thread(void *args) {
        pool_thread_impl((PoolData *) args);
        return nullptr;
    }

    static void pool_dtor(void *tls) {
        PoolData *data = (PoolData *) tls;
        // Should already be set nullptr.
        REQUIRE_EQ(nullptr, fiber_getspecific(data->key));
        if (++data->seq < data->end_seq) {
            REQUIRE_EQ(0, fiber_setspecific(data->key, data));
        }
    }

    TEST_CASE("KeyTest, using_pool") {
        fiber_local_key key;
        REQUIRE_EQ(0, fiber_key_create(&key, pool_dtor));

        fiber_keytable_pool_t pool;
        REQUIRE_EQ(0, fiber_keytable_pool_init(&pool));
        REQUIRE_EQ(0, fiber_keytable_pool_size(&pool));

        FiberAttribute attr;
        REQUIRE_EQ(0, fiber_attr_init(&attr));
        attr.keytable_pool = &pool;

        FiberAttribute attr2 = attr;
        attr2.stack_type = StackType::STACK_TYPE_PTHREAD;

        PoolData fib_data = {key, nullptr, 0, 3};
        fiber_id_t fid;
        REQUIRE(fiber_start_urgent(&fid, &attr, pool_thread, &fib_data).ok());
        REQUIRE(fiber_join(fid, nullptr).ok());
        REQUIRE_EQ(0, fib_data.seq);
        REQUIRE_EQ(1, fiber_keytable_pool_size(&pool));

        PoolData fib2_data = {key, &fib_data, 0, 3};
        fiber_id_t fid2;
        REQUIRE(fiber_start_urgent(&fid2, &attr2, pool_thread, &fib2_data).ok());
        REQUIRE(fiber_join(fid2, nullptr).ok());
        REQUIRE_EQ(0, fib2_data.seq);
        REQUIRE_EQ(1, fiber_keytable_pool_size(&pool));

        REQUIRE_EQ(0, fiber_keytable_pool_destroy(&pool));

        REQUIRE_EQ(fib_data.end_seq, fib_data.seq);
        REQUIRE_EQ(0, fib2_data.seq);

        REQUIRE_EQ(0, fiber_key_delete(key));
    }

}  // namespace  turbo::fiber_internal
