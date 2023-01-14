
#ifndef  TURBO_VARIABLE_DETAIL_SAMPLER_H_
#define  TURBO_VARIABLE_DETAIL_SAMPLER_H_

#include <vector>
#include <mutex>
#include "turbo/container/linked_list.h"
#include "turbo/base/scoped_lock.h"           // TURBO_SCOPED_LOCK
#include "turbo/log/logging.h"               // TURBO_LOG()
#include "turbo/container/bounded_queue.h"// bounded_queue
#include "turbo/base/type_traits.h"           // is_same
#include "turbo/times/time.h"                  // gettimeofday_us
#include "turbo/base/class_name.h"

namespace turbo {
    namespace metrics_detail {

        template<typename T>
        struct variable_sample {
            T data;
            int64_t time_us;

            variable_sample() : data(), time_us(0) {}

            variable_sample(const T &data2, int64_t time2) : data(data2), time_us(time2) {}
        };

        // The base class for all samplers whose take_sample() are called periodically.
        class variable_sampler : public turbo::container::link_node<variable_sampler> {
        public:
            variable_sampler();

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

            virtual ~variable_sampler();

            friend class sampler_collector;

            bool _used;
            // Sync destroy() and take_sample().
            std::mutex _mutex;
        };

        // Representing a non-existing operator so that we can test
        // is_same<Op, void_op>::value to write code for different branches.
        // The false branch should be removed by compiler at compile-time.
        struct void_op {
            template<typename T>
            T operator()(const T &, const T &) const {
                TURBO_CHECK(false) << "This function should never be called, abort";
                abort();
            }
        };

        // The sampler for reducer-alike variables.
        // The R should have following methods:
        //  - T reset();
        //  - T get_value();
        //  - Op op();
        //  - InvOp inv_op();
        template<typename R, typename T, typename Op, typename InvOp>
        class reducer_sampler : public variable_sampler {
        public:
            static const time_t MAX_SECONDS_LIMIT = 3600;

            explicit reducer_sampler(R *reducer)
                    : _reducer(reducer), _window_size(1) {

                // Invoked take_sample at begining so the value of the first second
                // would not be ignored
                take_sample();
            }

            ~reducer_sampler() {}

            void take_sample() override {
                // Make _q ready.
                // If _window_size is larger than what _q can hold, e.g. a larger
                // window<> is created after running of sampler, make _q larger.
                if ((size_t) _window_size + 1 > _q.capacity()) {
                    const size_t new_cap =
                            std::max(_q.capacity() * 2, (size_t) _window_size + 1);
                    const size_t memsize = sizeof(variable_sample<T>) * new_cap;
                    void *mem = malloc(memsize);
                    if (nullptr == mem) {
                        return;
                    }
                    turbo::container::bounded_queue<variable_sample<T> > new_q(
                            mem, memsize, turbo::container::OWNS_STORAGE);
                    variable_sample<T> tmp;
                    while (_q.pop(&tmp)) {
                        new_q.push(tmp);
                    }
                    new_q.swap(_q);
                }

                variable_sample<T> latest;
                if (std::is_same<InvOp, void_op>::value) {
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

            bool get_value(time_t window_size, variable_sample<T> *result) {
                if (window_size <= 0) {
                    TURBO_LOG(FATAL) << "Invalid window_size=" << window_size;
                    return false;
                }
                TURBO_SCOPED_LOCK(_mutex);
                if (_q.size() <= 1UL) {
                    // We need more samples to get reasonable result.
                    return false;
                }
                variable_sample<T> *oldest = _q.bottom(window_size);
                if (nullptr == oldest) {
                    oldest = _q.top();
                }
                variable_sample<T> *latest = _q.bottom();
                TURBO_DCHECK(latest != oldest);
                if (std::is_same<InvOp, void_op>::value) {
                    // No inverse op. Sum up all samples within the window.
                    result->data = latest->data;
                    for (int i = 1; true; ++i) {
                        variable_sample<T> *e = _q.bottom(i);
                        if (e == oldest) {
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
                    TURBO_LOG(ERROR) << "Invalid window_size=" << window_size;
                    return -1;
                }
                TURBO_SCOPED_LOCK(_mutex);
                if (window_size > _window_size) {
                    _window_size = window_size;
                }
                return 0;
            }

            void get_samples(std::vector<T> *samples, time_t window_size) {
                if (window_size <= 0) {
                    TURBO_LOG(FATAL) << "Invalid window_size=" << window_size;
                    return;
                }
                TURBO_SCOPED_LOCK(_mutex);
                if (_q.size() <= 1) {
                    // We need more samples to get reasonable result.
                    return;
                }
                variable_sample<T> *oldest = _q.bottom(window_size);
                if (nullptr == oldest) {
                    oldest = _q.top();
                }
                for (int i = 1; true; ++i) {
                    variable_sample<T> *e = _q.bottom(i);
                    if (e == oldest) {
                        break;
                    }
                    samples->push_back(e->data);
                }
            }

        private:
            R *_reducer;
            time_t _window_size;
            turbo::container::bounded_queue<variable_sample<T> > _q;
        };

    }  // namespace metrics_detail
}  // namespace turbo

#endif  // TURBO_VARIABLE_DETAIL_SAMPLER_H_
