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

#ifndef TURBO_FIBER_FIBER_SESSION_H_
#define TURBO_FIBER_FIBER_SESSION_H_

#include "turbo/fiber/internal/types.h"
#include "turbo/fiber/internal/fiber_session.h"
#include "turbo/status/status.h"
#include <functional>

namespace turbo {

    using fiber_session_t = turbo::fiber_internal::FiberSessionImpl;
    using fiber_session_list_t = turbo::fiber_internal::FiberSessionList;
    using turbo::fiber_internal::session_on_error;
    using turbo::fiber_internal::session_on_error_msg;
    using turbo::fiber_internal::fiber_mutex_t;

    static constexpr fiber_session_t INVALID_FIBER_SESSION = {0};


    inline int fiber_session_create(fiber_session_t *session) {
        return turbo::fiber_internal::fiber_session_create(session, nullptr, nullptr);
    }

    inline int fiber_session_create(fiber_session_t *session, void *data, const session_on_error &callback) {
        return turbo::fiber_internal::fiber_session_create(session, data, callback);
    }

    inline int fiber_session_create_msg(fiber_session_t *session, void *data, const session_on_error_msg &callback) {
        return turbo::fiber_internal::fiber_session_create2(session, data, callback);
    }

    inline int fiber_session_create_ranged(fiber_session_t *session, int range) {
        return turbo::fiber_internal::fiber_session_create_ranged(session, nullptr, nullptr, range);
    }

    inline int fiber_session_create_ranged(fiber_session_t *session, int range, void *data, const session_on_error &callback) {
        return turbo::fiber_internal::fiber_session_create_ranged(session, data, callback, range);
    }

    inline int
    fiber_session_create_ranged_msg(fiber_session_t *session, int range, void *data, const session_on_error_msg &callback) {
        return turbo::fiber_internal::fiber_session_create2_ranged(session, data, callback, range);
    }

    inline int fiber_session_cancel(fiber_session_t session) {
        return turbo::fiber_internal::fiber_session_cancel(session);
    }

    inline int fiber_session_join(fiber_session_t session) {
        return turbo::fiber_internal::fiber_session_join(session);
    }

    inline int fiber_session_about_to_destroy(fiber_session_t session) {
        return turbo::fiber_internal::fiber_session_about_to_destroy(session);
    }

    inline int fiber_session_trylock(fiber_session_t session) {
        return turbo::fiber_internal::fiber_session_trylock(session, nullptr);
    }

    inline int fiber_session_trylock(fiber_session_t session, void **data) {
        return turbo::fiber_internal::fiber_session_trylock(session, data);
    }

    inline int fiber_session_lock_verbose(fiber_session_t session, void **pdata,
                                          const char *location) {
        return turbo::fiber_internal::fiber_session_lock_verbose(session, pdata, location);
    }

    inline int fiber_session_error_verbose(fiber_session_t session, int error_code,
                                    const char *location) {
        return turbo::fiber_internal::fiber_session_error_verbose(session, error_code, location);
    }

    inline int fiber_session_lock_and_reset_range_verbose(fiber_session_t session, void **pdata,
                                                          int range, const char *location) {
        return turbo::fiber_internal::fiber_session_lock_and_reset_range_verbose(session, pdata, range, location);
    }

    inline int fiber_session_unlock_and_destroy(fiber_session_t session) {
        return turbo::fiber_internal::fiber_session_unlock_and_destroy(session);
    }

    inline int fiber_session_list_init(fiber_session_list_t *list,
                                       unsigned /*size*/,
                                       unsigned /*conflict_size*/) {
        return turbo::fiber_internal::fiber_session_list_init(list, 0, 0);
    }

    inline void fiber_session_list_destroy(fiber_session_list_t *list) {
        return turbo::fiber_internal::fiber_session_list_destroy(list);
    }

    inline int fiber_session_list_add(fiber_session_list_t *list, fiber_session_t id) {
        return turbo::fiber_internal::fiber_session_list_add(list, id);
    }

    inline void fiber_session_list_swap(fiber_session_list_t *dest,
                                        fiber_session_list_t *src) {
        return turbo::fiber_internal::fiber_session_list_swap(dest, src);
    }

    inline int fiber_session_list_reset(fiber_session_list_t *list, int error_code) {
        return turbo::fiber_internal::fiber_session_list_reset(list, error_code);
    }

    inline int fiber_session_list_reset_pthreadsafe(fiber_session_list_t *list, int error_code, std::mutex *mutex) {
        return turbo::fiber_internal::fiber_session_list_reset_pthreadsafe(list, error_code, mutex);
    }

    inline int fiber_session_list_reset_fibersafe(
            fiber_session_list_t *list, int error_code, FiberMutex *mutex) {
        return turbo::fiber_internal::fiber_session_list_reset_fibersafe(list, error_code, mutex);
    }

    inline int fiber_session_error_msg_verbose(fiber_session_t session, int error_code,
                                     const std::string &error_text,
                                     const char *location) {
        return turbo::fiber_internal::fiber_session_error2_verbose(session, error_code, error_text, location);
    }

    inline void fiber_session_status(fiber_session_t tn, std::ostream &os) {
        return turbo::fiber_internal::fiber_session_status(tn, os);
    }

    inline void fiber_session_pool_status(std::ostream &os) {
        return turbo::fiber_internal::fiber_session_pool_status(os);
    }



#define fiber_session_lock(id, pdata)                                      \
    fiber_session_lock_verbose(id, pdata, __FILE__ ":" TURBO_STRINGIFY(__LINE__))

#define fiber_session_error(id, err)                                        \
    fiber_session_error_verbose(id, err, __FILE__ ":" TURBO_STRINGIFY(__LINE__))


    inline int fiber_session_unlock(fiber_session_t session) {
        return turbo::fiber_internal::fiber_session_unlock(session);
    }

#define fiber_session_lock_and_reset_range(id, pdata, range)               \
    fiber_session_lock_and_reset_range_verbose(id, pdata, range,           \
                               __FILE__ ":" TURBO_STRINGIFY(__LINE__))

#define fiber_session_error_msg(id, ec, et)                                   \
    fiber_session_error_msg_verbose(id, ec, et, __FILE__ ":" TURBO_STRINGIFY(__LINE__))

}

namespace turbo {
    class FiberSession {
    public:
        FiberSession() = default;

        ~FiberSession();

        FiberSession(FiberSession &&other) noexcept;

        FiberSession &operator=(FiberSession &&other) noexcept;

        turbo::Status initialize();

        turbo::Status initialize(void *data, const session_on_error &callback);


    private:
        FiberSession(const FiberSession &) = delete;

        FiberSession &operator=(const FiberSession &) = delete;

    private:
        turbo::fiber_internal::FiberSessionImpl session_{0};
    };

    template<>
    struct formatter<turbo::fiber_session_t > {
        constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const turbo::fiber_session_t &session, FormatContext &ctx) {
            return format_to(ctx.out(), "FiberSession{{session_id={}}}", session.value);
        }
    };
}  // namespace turbo

#endif  // TURBO_FIBER_FIBER_SESSION_H_
