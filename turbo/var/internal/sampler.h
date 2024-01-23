// Copyright 2023 The Turbo Authors.
//
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

#ifndef  TURBO_VAR_INTERNAL_SAMPLER_H_
#define  TURBO_VAR_INTERNAL_SAMPLER_H_

#include <vector>
#include <mutex>
#include <type_traits>
#include "turbo/container/intrusive_list.h"
#include "turbo/base/internal/raw_logging.h"
#include "turbo/container/bounded_queue.h"
#include "turbo/meta/reflect.h"
#include "turbo/times/clock.h"

namespace turbo::var_internal {

    template<typename T>
    struct Sample {
        T data;
        int64_t time_us;

        Sample() : data(), time_us(0) {}

        Sample(const T &data2, int64_t time2) : data(data2), time_us(time2) {}
    };

    class Sampler : public turbo::intrusive_list_node {
    public:
        Sampler();

        // This function will be called every second(approximately) in a
        // dedicated thread if schedule() is called.
        virtual void take_sample() = 0;

        // Register this sampler globally so that take_sample() will be called
        // periodically.
        void schedule();

        // Call this function instead of delete to destroy the sampler. Deletion
        // of the sampler may be delayed for seconds.
        void destroy();

    protected:
        virtual ~Sampler();

        friend class SamplerCollector;

        bool _used;
        // Sync destroy() and take_sample().
        std::mutex _mutex;
    };

    struct VoidOp {
        template<typename T>
        T operator()(const T &, const T &) const {
            TURBO_RAW_LOG(FATAL, "This function should never be called, abort");
            abort();
        }
    };

    template<typename R, typename T, typename Op, typename InvOp>
    class ReducerSampler : public Sampler {
    public:
        static const time_t MAX_SECONDS_LIMIT = 3600;

        explicit ReducerSampler(R *reducer)
                : _reducer(reducer), _window_size(1) {

            // Invoked take_sample at begining so the value of the first second
            // would not be ignored
            take_sample();
        }

        ~ReducerSampler() {}

        void take_sample() override {
            // Make _q ready.
            // If _window_size is larger than what _q can hold, e.g. a larger
            // Window<> is created after running of sampler, make _q larger.
            if ((size_t) _window_size + 1 > _q.capacity()) {
                const size_t new_cap =
                        std::max(_q.capacity() * 2, (size_t) _window_size + 1);
                const size_t memsize = sizeof(Sample<T>) * new_cap;
                void *mem = ::malloc(memsize);
                if (nullptr == mem) {
                    return;
                }
                turbo::bounded_queue<Sample<T> > new_q(
                        mem, memsize, turbo::OWNS_STORAGE);
                Sample<T> tmp;
                while (_q.pop(&tmp)) {
                    new_q.push(tmp);
                }
                new_q.swap(_q);
            }

            Sample<T> latest;
            if (std::is_same<InvOp, VoidOp>::value) {
                // The operator can't be inversed.
                // We reset the reducer and save the result as a sample.
                // Suming up samples gives the result within a window.
                // In this case, get_value() of _reducer gives wrong answer and
                // should not be called.
                latest.data = _reducer->reset();
            } else {
                // The operator can be inversed.
                // We save the result as a sample.
                // Inversed operation between latest and oldest sample within a
                // window gives result.
                // get_value() of _reducer can still be called.
                latest.data = _reducer->get_value();
            }
            latest.time_us = turbo::get_current_time_micros();
            _q.elim_push(latest);
        }

        bool get_value(time_t window_size, Sample<T> *result) {
            if (window_size <= 0) {
                TURBO_RAW_LOG(FATAL, "Invalid window_size=%ld", window_size);
                return false;
            }
            std::unique_lock l(_mutex);
            if (_q.size() <= 1UL) {
                // We need more samples to get reasonable result.
                TURBO_RAW_LOG(WARNING, "Not enough samples, size=%ld", _q.size());
                return false;
            }
            Sample<T> *oldest = _q.bottom(window_size);
            if (nullptr == oldest) {
                oldest = _q.top();
            }
            Sample<T> *latest = _q.bottom();
            if (std::is_same<InvOp, VoidOp>::value) {
                // No inverse op. Sum up all samples within the window.
                result->data = latest->data;
                for (int i = 1; true; ++i) {
                    Sample<T> *e = _q.bottom(i);
                    if (e == oldest) {
                        TURBO_RAW_LOG(INFO, "ReducerSampler loop i=%d",i);
                        break;
                    }
                    _reducer->op()(result->data, e->data);
                }
            } else {
                // Diff the latest and oldest sample within the window.
                result->data = latest->data;
                _reducer->inv_op()(result->data, oldest->data);
            }
            result->time_us = latest->time_us - oldest->time_us;
            return true;
        }

        // Change the time window which can only go larger.
        int set_window_size(time_t window_size) {
            if (window_size <= 0 || window_size > MAX_SECONDS_LIMIT) {
                TURBO_RAW_LOG(ERROR, "Invalid window_size=%ld", window_size);
                return -1;
            }
            std::unique_lock l(_mutex);
            if (window_size > _window_size) {
                _window_size = window_size;
            }
            return 0;
        }

        void get_samples(std::vector<T> *samples, time_t window_size) {
            if (window_size <= 0) {
                TURBO_RAW_LOG(FATAL, "Invalid window_size=%ld", window_size);
                return;
            }
            std::unique_lock l(_mutex);
            if (_q.size() <= 1) {
                // We need more samples to get reasonable result.
                return;
            }
            Sample<T> *oldest = _q.bottom(window_size);
            if (nullptr == oldest) {
                oldest = _q.top();
            }
            for (int i = 1; true; ++i) {
                Sample<T> *e = _q.bottom(i);
                if (e == oldest) {
                    break;
                }
                samples->push_back(e->data);
            }
        }

    private:
        R *_reducer;
        time_t _window_size;
        turbo::bounded_queue<Sample<T> > _q;
    };

}  // namespace turbo::var_internal

#endif  // TURBO_VAR_INTERNAL_SAMPLER_H_
