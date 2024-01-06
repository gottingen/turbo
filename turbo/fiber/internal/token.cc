
#include <deque>
#include "turbo/log/logging.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/internal/mutex.h"
#include "turbo/fiber/internal/list_of_abafree_id.h"
#include "turbo/memory/resource_pool.h"
#include "turbo/fiber/internal/fiber.h"

namespace turbo::fiber_internal {

// This queue reduces the chance to allocate memory for deque
    template<typename T, int N>
    class SmallQueue {
    public:
        SmallQueue() : _begin(0), _size(0), _full(nullptr) {}

        void push(const T &val) {
            if (_full != nullptr && !_full->empty()) {
                _full->push_back(val);
            } else if (_size < N) {
                int tail = _begin + _size;
                if (tail >= N) {
                    tail -= N;
                }
                _c[tail] = val;
                ++_size;
            } else {
                if (_full == nullptr) {
                    _full = new std::deque<T>;
                }
                _full->push_back(val);
            }
        }

        bool pop(T *val) {
            if (_size > 0) {
                *val = _c[_begin];
                ++_begin;
                if (_begin >= N) {
                    _begin -= N;
                }
                --_size;
                return true;
            } else if (_full && !_full->empty()) {
                *val = _full->front();
                _full->pop_front();
                return true;
            }
            return false;
        }

        bool empty() const {
            return _size == 0 && (_full == nullptr || _full->empty());
        }

        size_t size() const {
            return _size + (_full ? _full->size() : 0);
        }

        void clear() {
            _size = 0;
            _begin = 0;
            if (_full) {
                _full->clear();
            }
        }

        ~SmallQueue() {
            delete _full;
            _full = nullptr;
        }

    private:TURBO_NON_COPYABLE(SmallQueue);

        int _begin;
        int _size;
        T _c[N];
        std::deque<T> *_full;
    };

    struct PendingError {
        fiber_token_t tn;
        int error_code;
        std::string error_text;
        const char *location;

        PendingError() : tn(INVALID_FIBER_TOKEN), error_code(0), location(nullptr) {}
    };

    struct TURBO_CACHE_LINE_ALIGNED token {
        // first_ver ~ locked_ver - 1: unlocked versions
        // locked_ver: locked
        // unlockable_ver: locked and about to be destroyed
        // contended_ver: locked and contended
        uint32_t first_ver;
        uint32_t locked_ver;
        turbo::SpinLock mutex;
        void *data;

        int (*on_error)(fiber_token_t, void *, int);

        int (*on_error2)(fiber_token_t, void *, int, const std::string &);

        const char *lock_location;
        uint32_t *event;
        uint32_t *join_futex;
        SmallQueue<PendingError, 2> pending_q;

        token() {
            // Although value of the event(as version part of fiber_token_t)
            // does not matter, we set it to 0 to make program more deterministic.
            event = turbo::fiber_internal::waitable_event_create_checked<uint32_t>();
            join_futex = turbo::fiber_internal::waitable_event_create_checked<uint32_t>();
            *event = 0;
            *join_futex = 0;
        }

        ~token() {
            turbo::fiber_internal::waitable_event_destroy(event);
            turbo::fiber_internal::waitable_event_destroy(join_futex);
        }

        inline bool has_version(uint32_t token_ver) const {
            return token_ver >= first_ver && token_ver < locked_ver;
        }

        inline uint32_t contended_ver() const { return locked_ver + 1; }

        inline uint32_t unlockable_ver() const { return locked_ver + 2; }

        inline uint32_t last_ver() const { return unlockable_ver(); }

        // also the next "first_ver"
        inline uint32_t end_ver() const { return last_ver() + 1; }
    };

    static_assert(sizeof(token) % 64 == 0, "sizeof token must align");

    typedef turbo::ResourceId<token> IdResourceId;

    inline fiber_token_t make_id(uint32_t version, IdResourceId slot) {
        const fiber_token_t tmp =
                {(((uint64_t) slot.value) << 32) | (uint64_t) version};
        return tmp;
    }

    inline IdResourceId get_slot(fiber_token_t tn) {
        const IdResourceId tmp = {(tn.value >> 32)};
        return tmp;
    }

    inline uint32_t get_version(fiber_token_t tn) {
        return (uint32_t) (tn.value & 0xFFFFFFFFul);
    }

    inline bool token_exists_with_true_negatives(fiber_token_t tn) {
        token *const meta = address_resource(get_slot(tn));
        if (meta == nullptr) {
            return false;
        }
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        return token_ver >= meta->first_ver && token_ver <= meta->last_ver();
    }

// required by unittest
    uint32_t token_value(fiber_token_t tn) {
        token *const meta = address_resource(get_slot(tn));
        if (meta != nullptr) {
            return *meta->event;
        }
        return 0;  // valid version never be zero
    }

    static int default_fiber_token_on_error(fiber_token_t tn, void *, int) {
        return fiber_token_unlock_and_destroy(tn);
    }

    static int default_fiber_token_on_error2(
            fiber_token_t tn, void *, int, const std::string &) {
        return fiber_token_unlock_and_destroy(tn);
    }

    void token_status(fiber_token_t tn, std::ostream &os) {
        turbo::fiber_internal::token *const meta = address_resource(turbo::fiber_internal::get_slot(tn));
        if (!meta) {
            os << "Invalid token=" << tn.value << '\n';
            return;
        }
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        uint32_t *event = meta->event;
        bool valid = true;
        void *data = nullptr;
        int (*on_error)(fiber_token_t, void *, int) = nullptr;
        int (*on_error2)(fiber_token_t, void *, int, const std::string &) = nullptr;
        uint32_t first_ver = 0;
        uint32_t locked_ver = 0;
        uint32_t unlockable_ver = 0;
        uint32_t contended_ver = 0;
        const char *lock_location = nullptr;
        SmallQueue<PendingError, 2> pending_q;
        uint32_t futex_value = 0;

        meta->mutex.lock();
        if (meta->has_version(token_ver)) {
            data = meta->data;
            on_error = meta->on_error;
            on_error2 = meta->on_error2;
            first_ver = meta->first_ver;
            locked_ver = meta->locked_ver;
            unlockable_ver = meta->unlockable_ver();
            contended_ver = meta->contended_ver();
            lock_location = meta->lock_location;
            const size_t size = meta->pending_q.size();
            for (size_t i = 0; i < size; ++i) {
                PendingError front;
                meta->pending_q.pop(&front);
                meta->pending_q.push(front);
                pending_q.push(front);
            }
            futex_value = *event;
        } else {
            valid = false;
        }
        meta->mutex.unlock();

        if (valid) {
            os << "First token: "
               << turbo::fiber_internal::make_id(first_ver, turbo::fiber_internal::get_slot(tn)).value << '\n'
               << "Range: " << locked_ver - first_ver << '\n'
               << "Status: ";
            if (futex_value != first_ver) {
                os << "LOCKED at " << lock_location;
                if (futex_value == contended_ver) {
                    os << " (CONTENDED)";
                } else if (futex_value == unlockable_ver) {
                    os << " (ABOUT TO DESTROY)";
                } else {
                    os << " (UNCONTENDED)";
                }
            } else {
                os << "UNLOCKED";
            }
            os << "\nPendingQ:";
            if (pending_q.empty()) {
                os << " EMPTY";
            } else {
                const size_t size = pending_q.size();
                for (size_t i = 0; i < size; ++i) {
                    PendingError front;
                    pending_q.pop(&front);
                    os << " (" << front.location << "/E" << turbo::format("{}", front.error_code)
                       << '/' << front.error_text << ')';
                }
            }
            if (on_error) {
                if (on_error == default_fiber_token_on_error) {
                    os << "\nOnError: unlock_and_destroy";
                } else {
                    os << "\nOnError: " << (void *) on_error;
                }
            } else {
                if (on_error2 == default_fiber_token_on_error2) {
                    os << "\nOnError2: unlock_and_destroy";
                } else {
                    os << "\nOnError2: " << (void *) on_error2;
                }
            }
            os << "\nData: " << data;
        } else {
            os << "Invalid token=" << tn.value;
        }
        os << '\n';
    }

    void token_pool_status(std::ostream &os) {
        os << turbo::describe_resources<token>() << '\n';
    }

    struct token_traits {
        static const size_t BLOCK_SIZE = 63;
        static const size_t MAX_ENTRIES = 100000;
        static const fiber_token_t TOKEN_INIT;

        static bool exists(fiber_token_t tn) { return turbo::fiber_internal::token_exists_with_true_negatives(tn); }
    };

    const fiber_token_t token_traits::TOKEN_INIT = INVALID_FIBER_TOKEN;

    typedef ListOfABAFreeId<fiber_token_t, token_traits> token_list;

    struct token_resetter {
        explicit token_resetter(int ec, const std::string &et)
                : _error_code(ec), _error_text(et) {}

        void operator()(fiber_token_t &tn) const {
            fiber_token_error2_verbose(
                    tn, _error_code, _error_text, turbo::format("{}:{}", __FILE__, __LINE__).c_str());
            tn = INVALID_FIBER_TOKEN;
        }

    private:
        int _error_code;
        const std::string &_error_text;
    };

    size_t get_sizes(const fiber_token_list_t *list, size_t *cnt, size_t n) {
        if (list->impl == nullptr) {
            return 0;
        }
        return static_cast<turbo::fiber_internal::token_list *>(list->impl)->get_sizes(cnt, n);
    }

    const int TOKEN_MAX_RANGE = 1024;

    static int token_create_impl(
            fiber_token_t *tn, void *data,
            int (*on_error)(fiber_token_t, void *, int),
            int (*on_error2)(fiber_token_t, void *, int, const std::string &)) {
        IdResourceId slot;
        token *const meta = get_resource(&slot);
        if (meta) {
            meta->data = data;
            meta->on_error = on_error;
            meta->on_error2 = on_error2;
            TLOG_CHECK(meta->pending_q.empty());
            uint32_t *event = meta->event;
            if (0 == *event || *event + TOKEN_MAX_RANGE + 2 < *event) {
                // Skip 0 so that fiber_token_t is never 0
                // avoid overflow to make comparisons simpler.
                *event = 1;
            }
            *meta->join_futex = *event;
            meta->first_ver = *event;
            meta->locked_ver = *event + 1;
            *tn = make_id(*event, slot);
            return 0;
        }
        return ENOMEM;
    }

    static int token_create_ranged_impl(
            fiber_token_t *tn, void *data,
            int (*on_error)(fiber_token_t, void *, int),
            int (*on_error2)(fiber_token_t, void *, int, const std::string &),
            int range) {
        if (range < 1 || range > TOKEN_MAX_RANGE) {
            TLOG_CRITICAL_IF(range < 1, "range must be positive, actually {}", range);
            TLOG_CRITICAL_IF(range > TOKEN_MAX_RANGE, "max of range is {} , actually {}", TOKEN_MAX_RANGE, range);
            return EINVAL;
        }
        IdResourceId slot;
        token *const meta = get_resource(&slot);
        if (meta) {
            meta->data = data;
            meta->on_error = on_error;
            meta->on_error2 = on_error2;
            TLOG_CHECK(meta->pending_q.empty());
            uint32_t *event = meta->event;
            if (0 == *event || *event + TOKEN_MAX_RANGE + 2 < *event) {
                // Skip 0 so that fiber_token_t is never 0
                // avoid overflow to make comparisons simpler.
                *event = 1;
            }
            *meta->join_futex = *event;
            meta->first_ver = *event;
            meta->locked_ver = *event + range;
            *tn = make_id(*event, slot);
            return 0;
        }
        return ENOMEM;
    }


    int fiber_token_create(
            fiber_token_t *tn, void *data,
            int (*on_error)(fiber_token_t, void *, int)) {
        return turbo::fiber_internal::token_create_impl(
                tn, data,
                (on_error ? on_error : turbo::fiber_internal::default_fiber_token_on_error), nullptr);
    }

    int fiber_token_create_ranged(fiber_token_t *tn, void *data,
                                  int (*on_error)(fiber_token_t, void *, int),
                                  int range) {
        return turbo::fiber_internal::token_create_ranged_impl(
                tn, data,
                (on_error ? on_error : turbo::fiber_internal::default_fiber_token_on_error),
                nullptr, range);
    }

    int fiber_token_lock_and_reset_range_verbose(
            fiber_token_t tn, void **pdata, int range, const char *location) {
        turbo::fiber_internal::token *const meta = address_resource(turbo::fiber_internal::get_slot(tn));
        if (!meta) {
            return EINVAL;
        }
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        uint32_t *event = meta->event;
        bool ever_contended = false;
        meta->mutex.lock();
        while (meta->has_version(token_ver)) {
            if (*event == meta->first_ver) {
                // contended locker always wakes up the event at unlock.
                meta->lock_location = location;
                if (range == 0) {
                    // fast path
                } else if (range < 0 ||
                           range > turbo::fiber_internal::TOKEN_MAX_RANGE ||
                           range + meta->first_ver <= meta->locked_ver) {
                    TLOG_CRITICAL_IF(range < 1, "range must be positive, actually {}", range);
                    TLOG_CRITICAL_IF(range > turbo::fiber_internal::TOKEN_MAX_RANGE, "max of range is {} , actually {}",
                                     turbo::fiber_internal::TOKEN_MAX_RANGE, range);
                } else {
                    meta->locked_ver = meta->first_ver + range;
                }
                *event = (ever_contended ? meta->contended_ver() : meta->locked_ver);
                meta->mutex.unlock();
                if (pdata) {
                    *pdata = meta->data;
                }
                return 0;
            } else if (*event != meta->unlockable_ver()) {
                *event = meta->contended_ver();
                uint32_t expected_ver = *event;
                meta->mutex.unlock();
                ever_contended = true;
                auto rc = turbo::fiber_internal::waitable_event_wait(event, expected_ver, nullptr);
                if (!rc.ok() && !turbo::is_unavailable(rc)) {
                    return errno;
                }
                meta->mutex.lock();
            } else { // fiber_token_about_to_destroy was called.
                meta->mutex.unlock();
                return EPERM;
            }
        }
        meta->mutex.unlock();
        return EINVAL;
    }

    int fiber_token_error_verbose(fiber_token_t tn, int error_code,
                                  const char *location) {
        return fiber_token_error2_verbose(tn, error_code, std::string(), location);
    }

    int fiber_token_about_to_destroy(fiber_token_t tn) {
        turbo::fiber_internal::token *const meta = address_resource(turbo::fiber_internal::get_slot(tn));
        if (!meta) {
            return EINVAL;
        }
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        uint32_t *event = meta->event;
        meta->mutex.lock();
        if (!meta->has_version(token_ver)) {
            meta->mutex.unlock();
            return EINVAL;
        }
        if (*event == meta->first_ver) {
            meta->mutex.unlock();
            TLOG_CRITICAL("fiber_token={} is not locked!", tn.value);
            return EPERM;
        }
        const bool contended = (*event == meta->contended_ver());
        *event = meta->unlockable_ver();
        meta->mutex.unlock();
        if (contended) {
            // wake up all waiting lockers.
            turbo::fiber_internal::waitable_event_wake_except(event, 0);
        }
        return 0;
    }

    int fiber_token_cancel(fiber_token_t tn) {
        turbo::fiber_internal::token *const meta = address_resource(turbo::fiber_internal::get_slot(tn));
        if (!meta) {
            return EINVAL;
        }
        uint32_t *event = meta->event;
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        meta->mutex.lock();
        if (!meta->has_version(token_ver)) {
            meta->mutex.unlock();
            return EINVAL;
        }
        if (*event != meta->first_ver) {
            meta->mutex.unlock();
            return EPERM;
        }
        *event = meta->end_ver();
        meta->first_ver = *event;
        meta->locked_ver = *event;
        meta->mutex.unlock();
        return_resource(turbo::fiber_internal::get_slot(tn));
        return 0;
    }

    int fiber_token_join(fiber_token_t tn) {
        const turbo::fiber_internal::IdResourceId slot = turbo::fiber_internal::get_slot(tn);
        turbo::fiber_internal::token *const meta = address_resource(slot);
        if (!meta) {
            // The token is not created yet, this join is definitely wrong.
            return EINVAL;
        }
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        uint32_t *join_futex = meta->join_futex;
        while (1) {
            meta->mutex.lock();
            const bool has_ver = meta->has_version(token_ver);
            const uint32_t expected_ver = *join_futex;
            meta->mutex.unlock();
            if (!has_ver) {
                break;
            }
            auto rs = turbo::fiber_internal::waitable_event_wait(join_futex, expected_ver, nullptr);
            if (!rs.ok() && !turbo::is_unavailable(rs)) {
                return errno;
            }
        }
        return 0;
    }

    int fiber_token_trylock(fiber_token_t tn, void **pdata) {
        turbo::fiber_internal::token *const meta = address_resource(turbo::fiber_internal::get_slot(tn));
        if (!meta) {
            return EINVAL;
        }
        uint32_t *event = meta->event;
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        meta->mutex.lock();
        if (!meta->has_version(token_ver)) {
            meta->mutex.unlock();
            return EINVAL;
        }
        if (*event != meta->first_ver) {
            meta->mutex.unlock();
            return EBUSY;
        }
        *event = meta->locked_ver;
        meta->mutex.unlock();
        if (pdata != nullptr) {
            *pdata = meta->data;
        }
        return 0;
    }

    int fiber_token_lock_verbose(fiber_token_t tn, void **pdata,
                                 const char *location) {
        return fiber_token_lock_and_reset_range_verbose(tn, pdata, 0, location);
    }

    int fiber_token_unlock(fiber_token_t tn) {
        turbo::fiber_internal::token *const meta = address_resource(turbo::fiber_internal::get_slot(tn));
        if (!meta) {
            return EINVAL;
        }
        uint32_t *event = meta->event;
        // Release fence makes sure all changes made before signal visible to
        // woken-up waiters.
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        meta->mutex.lock();
        if (!meta->has_version(token_ver)) {
            meta->mutex.unlock();
            TLOG_CRITICAL("Invalid fiber_token={}", tn.value);
            return EINVAL;
        }
        if (*event == meta->first_ver) {
            meta->mutex.unlock();
            TLOG_CRITICAL("fiber_token={} is not locked!", tn.value);
            return EPERM;
        }
        turbo::fiber_internal::PendingError front;
        if (meta->pending_q.pop(&front)) {
            meta->lock_location = front.location;
            meta->mutex.unlock();
            if (meta->on_error) {
                return meta->on_error(front.tn, meta->data, front.error_code);
            } else {
                return meta->on_error2(front.tn, meta->data, front.error_code,
                                       front.error_text);
            }
        } else {
            const bool contended = (*event == meta->contended_ver());
            *event = meta->first_ver;
            meta->mutex.unlock();
            if (contended) {
                // We may wake up already-reused token, but that's OK.
                turbo::fiber_internal::waitable_event_wake(event);
            }
            return 0;
        }
    }

    int fiber_token_unlock_and_destroy(fiber_token_t tn) {
        turbo::fiber_internal::token *const meta = address_resource(turbo::fiber_internal::get_slot(tn));
        if (!meta) {
            return EINVAL;
        }
        uint32_t *event = meta->event;
        uint32_t *join_futex = meta->join_futex;
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        meta->mutex.lock();
        if (!meta->has_version(token_ver)) {
            meta->mutex.unlock();
            TLOG_CRITICAL("Invalid fiber_token={}", tn.value);
            return EINVAL;
        }
        if (*event == meta->first_ver) {
            meta->mutex.unlock();
            TLOG_CRITICAL("fiber_token={} is not locked!", tn.value);
            return EPERM;
        }
        const uint32_t next_ver = meta->end_ver();
        *event = next_ver;
        *join_futex = next_ver;
        meta->first_ver = next_ver;
        meta->locked_ver = next_ver;
        meta->pending_q.clear();
        meta->mutex.unlock();
        // Notice that waitable_event_wake* returns # of woken-up, not successful or not.
        turbo::fiber_internal::waitable_event_wake_except(event, 0);
        turbo::fiber_internal::waitable_event_wake_all(join_futex);
        return_resource(turbo::fiber_internal::get_slot(tn));
        return 0;
    }

    int fiber_token_list_init(fiber_token_list_t *list,
                              unsigned /*size*/,
                              unsigned /*conflict_size*/) {
        list->impl = nullptr;  // create on demand.
        // Set unused fields to zero as well.
        list->head = 0;
        list->size = 0;
        list->conflict_head = 0;
        list->conflict_size = 0;
        return 0;
    }

    void fiber_token_list_destroy(fiber_token_list_t *list) {
        delete static_cast<turbo::fiber_internal::token_list *>(list->impl);
        list->impl = nullptr;
    }

    int fiber_token_list_add(fiber_token_list_t *list, fiber_token_t tn) {
        if (list->impl == nullptr) {
            list->impl = new(std::nothrow) turbo::fiber_internal::token_list;
            if (nullptr == list->impl) {
                return ENOMEM;
            }
        }
        return static_cast<turbo::fiber_internal::token_list *>(list->impl)->add(tn);
    }

    int fiber_token_list_reset(fiber_token_list_t *list, int error_code) {
        return fiber_token_reset2(list, error_code, std::string());
    }

    void fiber_token_list_swap(fiber_token_list_t *list1,
                               fiber_token_list_t *list2) {
        std::swap(list1->impl, list2->impl);
    }

    int fiber_token_list_reset_pthreadsafe(fiber_token_list_t *list, int error_code,
                                           pthread_mutex_t *mutex) {
        return fiber_token_list_reset2_pthreadsafe(
                list, error_code, std::string(), mutex);
    }

    int fiber_token_list_reset_fibersafe(fiber_token_list_t *list, int error_code,
                                         fiber_mutex_t *mutex) {
        return fiber_token_list_reset2_fibersafe(
                list, error_code, std::string(), mutex);
    }


    int fiber_token_create2(
            fiber_token_t *tn, void *data,
            int (*on_error)(fiber_token_t, void *, int, const std::string &)) {
        return turbo::fiber_internal::token_create_impl(
                tn, data, nullptr,
                (on_error ? on_error : turbo::fiber_internal::default_fiber_token_on_error2));
    }

    int fiber_token_create2_ranged(
            fiber_token_t *tn, void *data,
            int (*on_error)(fiber_token_t, void *, int, const std::string &),
            int range) {
        return turbo::fiber_internal::token_create_ranged_impl(
                tn, data, nullptr,
                (on_error ? on_error : turbo::fiber_internal::default_fiber_token_on_error2), range);
    }

    int fiber_token_error2_verbose(fiber_token_t tn, int error_code,
                                   const std::string &error_text,
                                   const char *location) {
        turbo::fiber_internal::token *const meta = address_resource(turbo::fiber_internal::get_slot(tn));
        if (!meta) {
            return EINVAL;
        }
        const uint32_t token_ver = turbo::fiber_internal::get_version(tn);
        uint32_t *event = meta->event;
        meta->mutex.lock();
        if (!meta->has_version(token_ver)) {
            meta->mutex.unlock();
            return EINVAL;
        }
        if (*event == meta->first_ver) {
            *event = meta->locked_ver;
            meta->lock_location = location;
            meta->mutex.unlock();
            if (meta->on_error) {
                return meta->on_error(tn, meta->data, error_code);
            } else {
                return meta->on_error2(tn, meta->data, error_code, error_text);
            }
        } else {
            turbo::fiber_internal::PendingError e;
            e.tn = tn;
            e.error_code = error_code;
            e.error_text = error_text;
            e.location = location;
            meta->pending_q.push(e);
            meta->mutex.unlock();
            return 0;
        }
    }

    int fiber_token_reset2(fiber_token_list_t *list,
                           int error_code,
                           const std::string &error_text) {
        if (list->impl != nullptr) {
            static_cast<turbo::fiber_internal::token_list *>(list->impl)->apply(
                    turbo::fiber_internal::token_resetter(error_code, error_text));
        }
        return 0;
    }

    int fiber_token_list_reset2_pthreadsafe(fiber_token_list_t *list,
                                            int error_code,
                                            const std::string &error_text,
                                            pthread_mutex_t *mutex) {
        if (mutex == nullptr) {
            return EINVAL;
        }
        if (list->impl == nullptr) {
            return 0;
        }
        fiber_token_list_t tmplist;
        const int rc = fiber_token_list_init(&tmplist, 0, 0);
        if (rc != 0) {
            return rc;
        }
        // Swap out the list then reset. The critical section is very small.
        pthread_mutex_lock(mutex);
        std::swap(list->impl, tmplist.impl);
        pthread_mutex_unlock(mutex);
        const int rc2 = fiber_token_reset2(&tmplist, error_code, error_text);
        fiber_token_list_destroy(&tmplist);
        return rc2;
    }

    int fiber_token_list_reset2_fibersafe(fiber_token_list_t *list,
                                          int error_code,
                                          const std::string &error_text,
                                          fiber_mutex_t *mutex) {
        if (mutex == nullptr) {
            return EINVAL;
        }
        if (list->impl == nullptr) {
            return 0;
        }
        fiber_token_list_t tmplist;
        const int rc = fiber_token_list_init(&tmplist, 0, 0);
        if (rc != 0) {
            return rc;
        }
        // Swap out the list then reset. The critical section is very small.
        TURBO_UNUSED(turbo::fiber_internal::fiber_mutex_lock(mutex));
        std::swap(list->impl, tmplist.impl);
        turbo::fiber_internal::fiber_mutex_unlock(mutex);
        const int rc2 = fiber_token_reset2(&tmplist, error_code, error_text);
        fiber_token_list_destroy(&tmplist);
        return rc2;
    }
}  // namespace turbo::fiber_internal