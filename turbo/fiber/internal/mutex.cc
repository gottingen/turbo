// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: Sun Aug  3 12:46:15 CST 2014

#include <pthread.h>
#include <execinfo.h>
#include "turbo/files/filesystem.h"
#include <dlfcn.h>                               // dlsym
#include <fcntl.h>                               // O_RDONLY
#include "turbo/base/static_atomic.h"
#include "turbo/metrics/all.h"
#include "turbo/metrics/collector.h"
#include "turbo/container/flat_map.h"
#include "turbo/io/cord_buf.h"
#include "turbo/base/fd_guard.h"
#include <memory>
#include "turbo/hash/murmurhash3.h"
#include "turbo/log/logging.h"
#include "turbo/memory/object_pool.h"
#include "turbo/fiber/internal/waitable_event.h"                       // waitable_event_*
#include "turbo/fiber/internal/processor.h"                   // cpu_relax, barrier
#include "turbo/fiber/internal/mutex.h"                       // fiber_mutex_t
#include "turbo/fiber/internal/sys_futex.h"
#include "turbo/fiber/internal/log.h"

extern "C" {
extern void *_dl_sym(void *handle, const char *symbol, void *caller);
}

namespace turbo::fiber_internal {
// Warm up backtrace before main().
    void *dummy_buf[4];
    const int TURBO_ALLOW_UNUSED
    dummy_bt = backtrace(dummy_buf, TURBO_ARRAY_SIZE(dummy_buf));

// For controlling contentions collected per second.
    static turbo::CollectorSpeedLimit g_cp_sl = VARIABLE_COLLECTOR_SPEED_LIMIT_INITIALIZER;

    const size_t MAX_CACHED_CONTENTIONS = 512;
// Skip frames which are always same: the unlock function and submit_contention()
    const int SKIPPED_STACK_FRAMES = 2;

    struct SampledContention : public turbo::Collected {
        // time taken by lock and unlock, normalized according to sampling_range
        int64_t duration_ns;
        // number of samples, normalized according to to sampling_range
        double count;
        int nframes;          // #elements in stack
        void *stack[26];      // backtrace.

        // Implement turbo::Collected
        void dump_and_destroy(size_t round) override;

        void destroy() override;

        turbo::CollectorSpeedLimit *speed_limit() override { return &g_cp_sl; }

        // For combining samples with hashmap.
        size_t hash_code() const {
            if (nframes == 0) {
                return 0;
            }
            uint32_t code = 1;
            uint32_t seed = nframes;
            turbo::hash::MurmurHash3_x86_32(stack, sizeof(void *) * nframes, seed, &code);
            return code;
        }
    };

    static_assert(sizeof(SampledContention) == 256, "be_friendly_to_allocator");

// Functor to compare contentions.
    struct ContentionEqual {
        bool operator()(const SampledContention *c1,
                        const SampledContention *c2) const {
            return c1->hash_code() == c2->hash_code() &&
                   c1->nframes == c2->nframes &&
                   memcmp(c1->stack, c2->stack, sizeof(void *) * c1->nframes) == 0;
        }
    };

// Functor to hash contentions.
    struct ContentionHash {
        size_t operator()(const SampledContention *c) const {
            return c->hash_code();
        }
    };

// The global context for contention profiler.
    class ContentionProfiler {
    public:
        typedef turbo::container::FlatMap<SampledContention *, SampledContention *,
                ContentionHash, ContentionEqual> ContentionMap;

        explicit ContentionProfiler(const char *name);

        ~ContentionProfiler();

        void dump_and_destroy(SampledContention *c);

        // Write buffered data into resulting file. If `ending' is true, append
        // content of /proc/self/maps and retry writing until buffer is empty.
        void flush_to_disk(bool ending);

        void init_if_needed();

    private:
        bool _init;  // false before first dump_and_destroy is called
        bool _first_write;      // true if buffer was not written to file yet.
        std::string _filename;  // the file storing profiling result.
        turbo::cord_buf _disk_buf;  // temp buf before saving the file.
        ContentionMap _dedup_map; // combining same samples to make result smaller.
    };

    ContentionProfiler::ContentionProfiler(const char *name)
            : _init(false), _first_write(true), _filename(name) {
    }

    ContentionProfiler::~ContentionProfiler() {
        if (!_init) {
            // Don't write file if dump_and_destroy was never called. We may create
            // such instances in ContentionProfilerStart.
            return;
        }
        flush_to_disk(true);
    }

    void ContentionProfiler::init_if_needed() {
        if (!_init) {
            // Already output nanoseconds, always set cycles/second to 1000000000.
            _disk_buf.append("--- contention\ncycles/second=1000000000\n");
            TURBO_CHECK_EQ(0, _dedup_map.init(1024, 60));
            _init = true;
        }
    }

    void ContentionProfiler::dump_and_destroy(SampledContention *c) {
        init_if_needed();
        // Categorize the contention.
        SampledContention **p_c2 = _dedup_map.seek(c);
        if (p_c2) {
            // Most contentions are caused by several hotspots, this should be
            // the common branch.
            SampledContention *c2 = *p_c2;
            c2->duration_ns += c->duration_ns;
            c2->count += c->count;
            c->destroy();
        } else {
            _dedup_map.insert(c, c);
        }
        if (_dedup_map.size() > MAX_CACHED_CONTENTIONS) {
            flush_to_disk(false);
        }
    }

    void ContentionProfiler::flush_to_disk(bool ending) {
        BT_VLOG << "flush_to_disk(ending=" << ending << ")";

        // Serialize contentions in _dedup_map into _disk_buf.
        if (!_dedup_map.empty()) {
            BT_VLOG << "dedup_map=" << _dedup_map.size();
            turbo::cord_buf_builder os;
            for (ContentionMap::const_iterator
                         it = _dedup_map.begin(); it != _dedup_map.end(); ++it) {
                SampledContention *c = it->second;
                os << c->duration_ns << ' ' << (size_t) ceil(c->count) << " @";
                for (int i = SKIPPED_STACK_FRAMES; i < c->nframes; ++i) {
                    os << ' ' << (void *) c->stack[i];
                }
                os << '\n';
                c->destroy();
            }
            _dedup_map.clear();
            _disk_buf.append(os.buf());
        }

        // Append /proc/self/maps to the end of the contention file, required by
        // pprof.pl, otherwise the functions in sys libs are not interpreted.
        if (ending) {
            BT_VLOG << "Append /proc/self/maps";
            // Failures are not critical, don't return directly.
            turbo::IOPortal mem_maps;
            const turbo::base::fd_guard fd(open("/proc/self/maps", O_RDONLY));
            if (fd >= 0) {
                while (true) {
                    ssize_t nr = mem_maps.append_from_file_descriptor(fd, 8192);
                    if (nr < 0) {
                        if (errno == EINTR) {
                            continue;
                        }
                        TURBO_PLOG(ERROR) << "Fail to read /proc/self/maps";
                        break;
                    }
                    if (nr == 0) {
                        _disk_buf.append(mem_maps);
                        break;
                    }
                }
            } else {
                TURBO_PLOG(ERROR) << "Fail to open /proc/self/maps";
            }
        }
        // Write _disk_buf into _filename
        std::error_code ec;
        turbo::file_path path(_filename);
        auto dir = path.parent_path();
        if (!turbo::create_directories(dir, ec)) {
            TURBO_LOG(ERROR) << "Fail to create directory=`" << dir.c_str()
                             << "', " << ec.message();
            return;
        }
        // Truncate on first write, append on later writes.
        int flag = O_APPEND;
        if (_first_write) {
            _first_write = false;
            flag = O_TRUNC;
        }
        turbo::base::fd_guard fd(open(_filename.c_str(), O_WRONLY | O_CREAT | flag, 0666));
        if (fd < 0) {
            TURBO_PLOG(ERROR) << "Fail to open " << _filename;
            return;
        }
        // Write once normally, write until empty in the end.
        do {
            ssize_t nw = _disk_buf.cut_into_file_descriptor(fd);
            if (nw < 0) {
                if (errno == EINTR) {
                    continue;
                }
                TURBO_PLOG(ERROR) << "Fail to write into " << _filename;
                return;
            }
            BT_VLOG << "Write " << nw << " bytes into " << _filename;
        } while (!_disk_buf.empty() && ending);
    }

// If contention profiler is on, this variable will be set with a valid
// instance. nullptr otherwise.
    static ContentionProfiler *TURBO_CACHELINE_ALIGNMENT
    g_cp = nullptr;
// Need this version to solve an issue that non-empty entries left by
// previous contention profilers should be detected and overwritten.
    static uint64_t g_cp_version = 0;
// Protecting accesss to g_cp.
    static pthread_mutex_t g_cp_mutex = PTHREAD_MUTEX_INITIALIZER;

// The map storing information for profiling pthread_mutex. Different from
// fiber_mutex, we can't save stuff into pthread_mutex, we neither can
// save the info in TLS reliably, since a mutex can be unlocked in a different
// thread from the one locked (although rare)
// This map must be very fast, since it's accessed inside the lock.
// Layout of the map:
//  * Align each entry by cacheline so that different threads do not collide.
//  * Hash the mutex into the map by its address. If the entry is occupied,
//    cancel sampling.
// The canceling rate should be small provided that programs are unlikely to
// lock a lot of mutexes simultaneously.
    const size_t MUTEX_MAP_SIZE = 1024;
    static_assert((MUTEX_MAP_SIZE
    & (MUTEX_MAP_SIZE - 1)) == 0, "must_be_power_of_2");
    struct TURBO_CACHELINE_ALIGNMENT MutexMapEntry{
            turbo::static_atomic<uint64_t> versioned_mutex;
            fiber_contention_site_t csite;
    };
    static MutexMapEntry g_mutex_map[MUTEX_MAP_SIZE] = {}; // zero-initialize

    void SampledContention::dump_and_destroy(size_t /*round*/) {
        if (g_cp) {
            // Must be protected with mutex to avoid race with deletion of ctx.
            // dump_and_destroy is called from dumping thread only so this mutex
            // is not contended at most of time.
            TURBO_SCOPED_LOCK(g_cp_mutex);
            if (g_cp) {
                g_cp->dump_and_destroy(this);
                return;
            }
        }
        destroy();
    }

    void SampledContention::destroy() {
        turbo::return_object(this);
    }

// Remember the conflict hashes for troubleshooting, should be 0 at most of time.
    static turbo::static_atomic<int64_t> g_nconflicthash = TURBO_STATIC_ATOMIC_INIT(0);

    static int64_t get_nconflicthash(void *) {
        return g_nconflicthash.load(std::memory_order_relaxed);
    }

// Start profiling contention.
    bool ContentionProfilerStart(const char *filename) {
        if (filename == nullptr) {
            TURBO_LOG(ERROR) << "Parameter [filename] is nullptr";
            return false;
        }
        // g_cp is also the flag marking start/stop.
        if (g_cp) {
            return false;
        }

        // Create related global variable lazily.
        static turbo::status_gauge<int64_t> g_nconflicthash_var
                ("contention_profiler_conflict_hash", get_nconflicthash, nullptr);
        static turbo::DisplaySamplingRatio g_sampling_ratio_var(
                "contention_profiler_sampling_ratio", &g_cp_sl);

        // Optimistic locking. A not-used ContentionProfiler does not write file.
        std::unique_ptr <ContentionProfiler> ctx(new ContentionProfiler(filename));
        {
            TURBO_SCOPED_LOCK(g_cp_mutex);
            if (g_cp) {
                return false;
            }
            g_cp = ctx.release();
            ++g_cp_version;  // invalidate non-empty entries that may exist.
        }
        return true;
    }

// Stop contention profiler.
    void ContentionProfilerStop() {
        ContentionProfiler *ctx = nullptr;
        if (g_cp) {
            std::unique_lock <pthread_mutex_t> mu(g_cp_mutex);
            if (g_cp) {
                ctx = g_cp;
                g_cp = nullptr;
                mu.unlock();

                // make sure it's initialiazed in case no sample was gathered,
                // otherwise nothing will be written and succeeding pprof will fail.
                ctx->init_if_needed();
                // Deletion is safe because usages of g_cp are inside g_cp_mutex.
                delete ctx;
                return;
            }
        }
        TURBO_LOG(ERROR) << "Contention profiler is not started!";
    }

    TURBO_FORCE_INLINE bool
    is_contention_site_valid(const fiber_contention_site_t &cs) {
        return cs.sampling_range;
    }

    TURBO_FORCE_INLINE void
    make_contention_site_invalid(fiber_contention_site_t *cs) {
        cs->sampling_range = 0;
    }

// Replace pthread_mutex_lock and pthread_mutex_unlock:
// First call to sys_pthread_mutex_lock sets sys_pthread_mutex_lock to the
// real function so that next calls go to the real function directly. This
// technique avoids calling pthread_once each time.
    typedef int (*MutexOp)(pthread_mutex_t *);

    int first_sys_pthread_mutex_lock(pthread_mutex_t *mutex);

    int first_sys_pthread_mutex_unlock(pthread_mutex_t *mutex);

    static MutexOp sys_pthread_mutex_lock = first_sys_pthread_mutex_lock;
    static MutexOp sys_pthread_mutex_unlock = first_sys_pthread_mutex_unlock;
    static pthread_once_t init_sys_mutex_lock_once = PTHREAD_ONCE_INIT;

    static void init_sys_mutex_lock() {
#if defined(TURBO_PLATFORM_LINUX)
        // TODO: may need dlvsym when GLIBC has multiple versions of a same symbol.
        // http://blog.fesnel.com/blog/2009/08/25/preloading-with-multiple-symbol-versions
        sys_pthread_mutex_lock = (MutexOp)_dl_sym(RTLD_NEXT, "pthread_mutex_lock", (void*)init_sys_mutex_lock);
        sys_pthread_mutex_unlock = (MutexOp)_dl_sym(RTLD_NEXT, "pthread_mutex_unlock", (void*)init_sys_mutex_lock);
#elif defined(TURBO_PLATFORM_OSX)
        // TODO: look workaround for dlsym on mac
        sys_pthread_mutex_lock = (MutexOp) dlsym(RTLD_NEXT, "pthread_mutex_lock");
        sys_pthread_mutex_unlock = (MutexOp) dlsym(RTLD_NEXT, "pthread_mutex_unlock");
#endif
    }

// Make sure pthread functions are ready before main().
    const int TURBO_ALLOW_UNUSED
    dummy = pthread_once(&init_sys_mutex_lock_once, init_sys_mutex_lock);

    int first_sys_pthread_mutex_lock(pthread_mutex_t *mutex) {
        pthread_once(&init_sys_mutex_lock_once, init_sys_mutex_lock);
        return sys_pthread_mutex_lock(mutex);
    }

    int first_sys_pthread_mutex_unlock(pthread_mutex_t *mutex) {
        pthread_once(&init_sys_mutex_lock_once, init_sys_mutex_lock);
        return sys_pthread_mutex_unlock(mutex);
    }

    inline uint64_t hash_mutex_ptr(const pthread_mutex_t *m) {
        return turbo::hash::fmix64((uint64_t) m);
    }

// Mark being inside locking so that pthread_mutex calls inside collecting
// code are never sampled, otherwise deadlock may occur.
    static __thread bool tls_inside_lock = false;

// Speed up with TLS:
//   Most pthread_mutex are locked and unlocked in the same thread. Putting
//   contention information in TLS avoids collisions that may occur in
//   g_mutex_map. However when user unlocks in another thread, the info cached
//   in the locking thread is not removed, making the space bloated. We use a
//   simple strategy to solve the issue: If a thread has enough thread-local
//   space to store the info, save it, otherwise save it in g_mutex_map. For
//   a program that locks and unlocks in the same thread and does not lock a
//   lot of mutexes simulateneously, this strategy always uses the TLS.
#ifndef DONT_SPEEDUP_PTHREAD_CONTENTION_PROFILER_WITH_TLS
    const int TLS_MAX_COUNT = 3;
    struct MutexAndContentionSite {
        pthread_mutex_t *mutex;
        fiber_contention_site_t csite;
    };
    struct TLSPthreadContentionSites {
        int count;
        uint64_t cp_version;
        MutexAndContentionSite list[TLS_MAX_COUNT];
    };
    static __thread TLSPthreadContentionSites tls_csites = {0, 0, {}};
#endif  // DONT_SPEEDUP_PTHREAD_CONTENTION_PROFILER_WITH_TLS

// Guaranteed in linux/win.
    const int PTR_BITS = 48;

    inline fiber_contention_site_t *
    add_pthread_contention_site(pthread_mutex_t *mutex) {
        MutexMapEntry & entry = g_mutex_map[hash_mutex_ptr(mutex) & (MUTEX_MAP_SIZE - 1)];
        turbo::static_atomic<uint64_t> &m = entry.versioned_mutex;
        uint64_t expected = m.load(std::memory_order_relaxed);
        // If the entry is not used or used by previous profiler, try to CAS it.
        if (expected == 0 ||
            (expected >> PTR_BITS) != (g_cp_version & ((1 << (64 - PTR_BITS)) - 1))) {
            uint64_t desired = (g_cp_version << PTR_BITS) | (uint64_t) mutex;
            if (m.compare_exchange_strong(
                    expected, desired, std::memory_order_acquire)) {
                return &entry.csite;
            }
        }
        g_nconflicthash.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
    }

    inline bool remove_pthread_contention_site(
            pthread_mutex_t *mutex, fiber_contention_site_t *saved_csite) {
        MutexMapEntry & entry = g_mutex_map[hash_mutex_ptr(mutex) & (MUTEX_MAP_SIZE - 1)];
        turbo::static_atomic<uint64_t> &m = entry.versioned_mutex;
        if ((m.load(std::memory_order_relaxed) & ((((uint64_t) 1) << PTR_BITS) - 1))
            != (uint64_t) mutex) {
            // This branch should be the most common case since most locks are
            // neither contended nor sampled. We have one memory indirection and
            // several bitwise operations here, the cost should be ~ 5-50ns
            return false;
        }
        // Although this branch is inside a contended lock, we should also make it
        // as simple as possible because altering the critical section too much
        // may make unpredictable impact to thread interleaving status, which
        // makes profiling result less accurate.
        *saved_csite = entry.csite;
        make_contention_site_invalid(&entry.csite);
        m.store(0, std::memory_order_release);
        return true;
    }

// Submit the contention along with the callsite('s stacktrace)
    void submit_contention(const fiber_contention_site_t &csite, int64_t now_ns) {
        tls_inside_lock = true;
        SampledContention *sc = turbo::get_object<SampledContention>();
        // Normalize duration_us and count so that they're addable in later
        // processings. Notice that sampling_range is adjusted periodically by
        // collecting thread.
        sc->duration_ns = csite.duration_ns * turbo::COLLECTOR_SAMPLING_BASE
                          / csite.sampling_range;
        sc->count = turbo::COLLECTOR_SAMPLING_BASE / (double) csite.sampling_range;
        sc->nframes = backtrace(sc->stack, TURBO_ARRAY_SIZE(sc->stack)); // may lock
        sc->submit(now_ns / 1000);  // may lock
        tls_inside_lock = false;
    }

    TURBO_FORCE_INLINE int pthread_mutex_lock_impl(pthread_mutex_t *mutex) {
        // Don't change behavior of lock when profiler is off.
        if (!g_cp ||
            // collecting code including backtrace() and submit() may call
            // pthread_mutex_lock and cause deadlock. Don't sample.
            tls_inside_lock) {
            return sys_pthread_mutex_lock(mutex);
        }
        // Don't slow down non-contended locks.
        int rc = pthread_mutex_trylock(mutex);
        if (rc != EBUSY) {
            return rc;
        }
        // Ask turbo::Collector if this (contended) locking should be sampled
        const size_t sampling_range = turbo::is_collectable(&g_cp_sl);

        fiber_contention_site_t *csite = nullptr;
#ifndef DONT_SPEEDUP_PTHREAD_CONTENTION_PROFILER_WITH_TLS
        TLSPthreadContentionSites &fast_alt = tls_csites;
        if (fast_alt.cp_version != g_cp_version) {
            fast_alt.cp_version = g_cp_version;
            fast_alt.count = 0;
        }
        if (fast_alt.count < TLS_MAX_COUNT) {
            MutexAndContentionSite &entry = fast_alt.list[fast_alt.count++];
            entry.mutex = mutex;
            csite = &entry.csite;
            if (!sampling_range) {
                make_contention_site_invalid(&entry.csite);
                return sys_pthread_mutex_lock(mutex);
            }
        }
#endif
        if (!sampling_range) {  // don't sample
            return sys_pthread_mutex_lock(mutex);
        }
        // Lock and monitor the waiting time.
        const int64_t start_ns = turbo::get_current_time_nanos();
        rc = sys_pthread_mutex_lock(mutex);
        if (!rc) { // Inside lock
            if (!csite) {
                csite = add_pthread_contention_site(mutex);
                if (csite == nullptr) {
                    return rc;
                }
            }
            csite->duration_ns = turbo::get_current_time_nanos() - start_ns;
            csite->sampling_range = sampling_range;
        } // else rare
        return rc;
    }

    TURBO_FORCE_INLINE int pthread_mutex_unlock_impl(pthread_mutex_t *mutex) {
        // Don't change behavior of unlock when profiler is off.
        if (!g_cp || tls_inside_lock) {
            // This branch brings an issue that an entry created by
            // add_pthread_contention_site may not be cleared. Thus we add a
            // 16-bit rolling version in the entry to find out such entry.
            return sys_pthread_mutex_unlock(mutex);
        }
        int64_t unlock_start_ns = 0;
        bool miss_in_tls = true;
        fiber_contention_site_t saved_csite = {0, 0};
#ifndef DONT_SPEEDUP_PTHREAD_CONTENTION_PROFILER_WITH_TLS
        TLSPthreadContentionSites &fast_alt = tls_csites;
        for (int i = fast_alt.count - 1; i >= 0; --i) {
            if (fast_alt.list[i].mutex == mutex) {
                if (is_contention_site_valid(fast_alt.list[i].csite)) {
                    saved_csite = fast_alt.list[i].csite;
                    unlock_start_ns = turbo::get_current_time_nanos();
                }
                fast_alt.list[i] = fast_alt.list[--fast_alt.count];
                miss_in_tls = false;
                break;
            }
        }
#endif
        // Check the map to see if the lock is sampled. Notice that we're still
        // inside critical section.
        if (miss_in_tls) {
            if (remove_pthread_contention_site(mutex, &saved_csite)) {
                unlock_start_ns = turbo::get_current_time_nanos();
            }
        }
        const int rc = sys_pthread_mutex_unlock(mutex);
        // [Outside lock]
        if (unlock_start_ns) {
            const int64_t unlock_end_ns = turbo::get_current_time_nanos();
            saved_csite.duration_ns += unlock_end_ns - unlock_start_ns;
            submit_contention(saved_csite, unlock_end_ns);
        }
        return rc;
    }

// Implement fiber_mutex_t related functions
    struct MutexInternal {
        turbo::static_atomic<unsigned char> locked;
        turbo::static_atomic<unsigned char> contended;
        unsigned short padding;
    };

    const MutexInternal MUTEX_CONTENDED_RAW = {{1}, {1}, 0};
    const MutexInternal MUTEX_LOCKED_RAW = {{1}, {0}, 0};
// Define as macros rather than constants which can't be put in read-only
// section and affected by initialization-order fiasco.
#define FIBER_MUTEX_CONTENDED (*(const unsigned*)&turbo::fiber_internal::MUTEX_CONTENDED_RAW)
#define FIBER_MUTEX_LOCKED (*(const unsigned*)&turbo::fiber_internal::MUTEX_LOCKED_RAW)

    static_assert(sizeof(unsigned) == sizeof(MutexInternal),
    "sizeof_mutex_internal_must_equal_unsigned");

    inline int mutex_lock_contended(fiber_mutex_t *m) {
        std::atomic<unsigned> *whole = (std::atomic<unsigned> *) m->event;
        while (whole->exchange(FIBER_MUTEX_CONTENDED) & FIBER_MUTEX_LOCKED) {
            if (turbo::fiber_internal::waitable_event_wait(whole, FIBER_MUTEX_CONTENDED, nullptr) < 0 &&
                errno != EWOULDBLOCK && errno != EINTR/*note*/) {
                // a mutex lock should ignore interrruptions in general since
                // user code is unlikely to check the return value.
                return errno;
            }
        }
        return 0;
    }

    inline int mutex_timedlock_contended(
            fiber_mutex_t *m, const struct timespec *__restrict abstime) {
        std::atomic<unsigned> *whole = (std::atomic<unsigned> *) m->event;
        while (whole->exchange(FIBER_MUTEX_CONTENDED) & FIBER_MUTEX_LOCKED) {
            if (turbo::fiber_internal::waitable_event_wait(whole, FIBER_MUTEX_CONTENDED, abstime) < 0 &&
                errno != EWOULDBLOCK && errno != EINTR/*note*/) {
                // a mutex lock should ignore interrruptions in general since
                // user code is unlikely to check the return value.
                return errno;
            }
        }
        return 0;
    }

    namespace internal {

        int FastPthreadMutex::lock_contended() {
            std::atomic<unsigned> *whole = (std::atomic < unsigned > *) & _futex;
            while (whole->exchange(FIBER_MUTEX_CONTENDED) & FIBER_MUTEX_LOCKED) {
                if (futex_wait_private(whole, FIBER_MUTEX_CONTENDED, nullptr) < 0
                    && errno != EWOULDBLOCK) {
                    return errno;
                }
            }
            return 0;
        }

        void FastPthreadMutex::lock() {
            turbo::fiber_internal::MutexInternal *split = (turbo::fiber_internal::MutexInternal *) &_futex;
            if (split->locked.exchange(1, std::memory_order_acquire)) {
                (void) lock_contended();
            }
        }

        bool FastPthreadMutex::try_lock() {
            turbo::fiber_internal::MutexInternal *split = (turbo::fiber_internal::MutexInternal *) &_futex;
            return !split->locked.exchange(1, std::memory_order_acquire);
        }

        void FastPthreadMutex::unlock() {
            std::atomic<unsigned> *whole = (std::atomic < unsigned > *) & _futex;
            const unsigned prev = whole->exchange(0, std::memory_order_release);
            // CAUTION: the mutex may be destroyed, check comments before waitable_event_create
            if (prev != FIBER_MUTEX_LOCKED) {
                futex_wake_private(whole, 1);
            }
        }

    } // namespace internal

} // namespace turbo::fiber_internal

extern "C" {

int fiber_mutex_init(fiber_mutex_t *__restrict m,
                     const fiber_mutexattr_t *__restrict) {
    turbo::fiber_internal::make_contention_site_invalid(&m->csite);
    m->event = turbo::fiber_internal::waitable_event_create_checked<unsigned>();
    if (!m->event) {
        return ENOMEM;
    }
    *m->event = 0;
    return 0;
}

int fiber_mutex_destroy(fiber_mutex_t *m) {
    turbo::fiber_internal::waitable_event_destroy(m->event);
    return 0;
}

int fiber_mutex_trylock(fiber_mutex_t *m) {
    turbo::fiber_internal::MutexInternal *split = (turbo::fiber_internal::MutexInternal *) m->event;
    if (!split->locked.exchange(1, std::memory_order_acquire)) {
        return 0;
    }
    return EBUSY;
}

int fiber_mutex_lock_contended(fiber_mutex_t *m) {
    return turbo::fiber_internal::mutex_lock_contended(m);
}

int fiber_mutex_lock(fiber_mutex_t *m) {
    turbo::fiber_internal::MutexInternal *split = (turbo::fiber_internal::MutexInternal *) m->event;
    if (!split->locked.exchange(1, std::memory_order_acquire)) {
        return 0;
    }
    // Don't sample when contention profiler is off.
    if (!turbo::fiber_internal::g_cp) {
        return turbo::fiber_internal::mutex_lock_contended(m);
    }
    // Ask Collector if this (contended) locking should be sampled.
    const size_t sampling_range = turbo::is_collectable(&turbo::fiber_internal::g_cp_sl);
    if (!sampling_range) { // Don't sample
        return turbo::fiber_internal::mutex_lock_contended(m);
    }
    // Start sampling.
    const int64_t start_ns = turbo::get_current_time_nanos();
    // NOTE: Don't modify m->csite outside lock since multiple threads are
    // still contending with each other.
    const int rc = turbo::fiber_internal::mutex_lock_contended(m);
    if (!rc) { // Inside lock
        m->csite.duration_ns = turbo::get_current_time_nanos() - start_ns;
        m->csite.sampling_range = sampling_range;
    } // else rare
    return rc;
}

int fiber_mutex_timedlock(fiber_mutex_t *__restrict m,
                          const struct timespec *__restrict abstime) {
    turbo::fiber_internal::MutexInternal *split = (turbo::fiber_internal::MutexInternal *) m->event;
    if (!split->locked.exchange(1, std::memory_order_acquire)) {
        return 0;
    }
    // Don't sample when contention profiler is off.
    if (!turbo::fiber_internal::g_cp) {
        return turbo::fiber_internal::mutex_timedlock_contended(m, abstime);
    }
    // Ask Collector if this (contended) locking should be sampled.
    const size_t sampling_range = turbo::is_collectable(&turbo::fiber_internal::g_cp_sl);
    if (!sampling_range) { // Don't sample
        return turbo::fiber_internal::mutex_timedlock_contended(m, abstime);
    }
    // Start sampling.
    const int64_t start_ns = turbo::get_current_time_nanos();
    // NOTE: Don't modify m->csite outside lock since multiple threads are
    // still contending with each other.
    const int rc = turbo::fiber_internal::mutex_timedlock_contended(m, abstime);
    if (!rc) { // Inside lock
        m->csite.duration_ns = turbo::get_current_time_nanos() - start_ns;
        m->csite.sampling_range = sampling_range;
    } else if (rc == ETIMEDOUT) {
        // Failed to lock due to ETIMEDOUT, submit the elapse directly.
        const int64_t end_ns = turbo::get_current_time_nanos();
        const fiber_contention_site_t csite = {end_ns - start_ns, sampling_range};
        turbo::fiber_internal::submit_contention(csite, end_ns);
    }
    return rc;
}

int fiber_mutex_unlock(fiber_mutex_t *m) {
    std::atomic<unsigned> *whole = (std::atomic<unsigned> *) m->event;
    fiber_contention_site_t saved_csite = {0, 0};
    if (turbo::fiber_internal::is_contention_site_valid(m->csite)) {
        saved_csite = m->csite;
        turbo::fiber_internal::make_contention_site_invalid(&m->csite);
    }
    const unsigned prev = whole->exchange(0, std::memory_order_release);
    // CAUTION: the mutex may be destroyed, check comments before waitable_event_create
    if (prev == FIBER_MUTEX_LOCKED) {
        return 0;
    }
    // Wakeup one waiter
    if (!turbo::fiber_internal::is_contention_site_valid(saved_csite)) {
        turbo::fiber_internal::waitable_event_wake(whole);
        return 0;
    }
    const int64_t unlock_start_ns = turbo::get_current_time_nanos();
    turbo::fiber_internal::waitable_event_wake(whole);
    const int64_t unlock_end_ns = turbo::get_current_time_nanos();
    saved_csite.duration_ns += unlock_end_ns - unlock_start_ns;
    turbo::fiber_internal::submit_contention(saved_csite, unlock_end_ns);
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *__mutex) {
    return turbo::fiber_internal::pthread_mutex_lock_impl(__mutex);
}
int pthread_mutex_unlock(pthread_mutex_t *__mutex) {
    return turbo::fiber_internal::pthread_mutex_unlock_impl(__mutex);
}

}  // extern "C"
