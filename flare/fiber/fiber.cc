
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/fiber/fiber.h"
#include "flare/fiber/internal/fiber.h"

namespace flare {
    fiber::fiber() : _save_error(0), _fid(INVALID_FIBER_ID), _detached(true) {

    }

    fiber::fiber(const attribute &attr, std::function<void *(void *)> &&fn, void *args)
            : _save_error(0),
              _fid(INVALID_FIBER_ID), _detached(false) {
        fiber_attribute tmp;
        tmp.stack_type = attr.stack_type;
        tmp.flags = attr.flags;
        tmp.keytable_pool = attr.keytable_pool;
        if (attr.policy == launch_policy::eImmediately) {
            _save_error = fiber_start_urgent(&_fid, &tmp, std::move(fn), args);
        } else {
            _save_error = fiber_start_background(&_fid, &tmp, std::move(fn), args);
        }
    }

    void fiber::join() {
        if(!_detached) {
            ::fiber_join(_fid, nullptr);
        }
        _fid = INVALID_FIBER_ID;
    }

    void fiber::detach() {
        _detached = true;
        _fid = INVALID_FIBER_ID;
    }

    bool fiber::stopped() const {
        if(_detached && _fid != INVALID_FIBER_ID)  {
            return ::fiber_stopped(_fid) == 0;
        }
        return true;
    }

    void fiber::stop() {
        ::fiber_stop(_fid);
    }

    fiber::~fiber() {
        if(!_detached && _fid != INVALID_FIBER_ID)  {
            stop();
            join();
        }
    }
}
