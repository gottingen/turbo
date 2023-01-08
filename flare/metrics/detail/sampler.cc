
#include "flare/times/time.h"
#include "flare/base/singleton_on_pthread_once.h"
#include "flare/metrics/variable_reducer.h"
#include "flare/metrics/detail/sampler.h"
#include "flare/metrics/gauge.h"
#include "flare/metrics/window.h"

namespace flare {
    namespace metrics_detail {

        const int WARN_NOSLEEP_THRESHOLD = 2;

        // Combine two circular linked list into one.
        struct CombineSampler {
            void operator()(variable_sampler *&s1, variable_sampler *s2) const {
                if (s2 == nullptr) {
                    return;
                }
                if (s1 == nullptr) {
                    s1 = s2;
                    return;
                }
                s1->insert_before_as_list(s2);
            }
        };

        // True iff pthread_atfork was called. The callback to atfork works for child
        // of child as well, no need to register in the child again.
        static bool registered_atfork = false;

        // Call take_sample() of all scheduled samplers.
        // This can be done with regular timer thread, but it's way too slow(global
        // contention + log(N) heap manipulations). We need it to be super fast so that
        // creation overhead of window<> is negliable.
        // The trick is to use variable_reducer<variable_sampler*, CombineSampler>. Each variable_sampler is
        // doubly linked, thus we can reduce multiple Samplers into one cicurlarly
        // doubly linked list, and multiple lists into larger lists. We create a
        // dedicated thread to periodically get_value() which is just the combined
        // list of Samplers. Waking through the list and call take_sample().
        // If a variable_sampler needs to be deleted, we just mark it as unused and the
        // deletion is taken place in the thread as well.
        class sampler_collector : public flare::variable_reducer<variable_sampler *, CombineSampler> {
        public:
            sampler_collector()
                    : _created(false), _stop(false), _cumulated_time_us(0) {
                create_sampling_thread();
            }

            ~sampler_collector() {
                if (_created) {
                    _stop = true;
                    pthread_join(_tid, nullptr);
                    _created = false;
                }
            }

        private:
            // Support for fork:
            // * The singleton can be null before forking, the child callback will not
            //   be registered.
            // * If the singleton is not null before forking, the child callback will
            //   be registered and the sampling thread will be re-created.
            // * A forked program can be forked again.

            static void child_callback_atfork() {
                flare::get_leaky_singleton<sampler_collector>()->after_forked_as_child();
            }

            void create_sampling_thread() {
                const int rc = pthread_create(&_tid, nullptr, sampling_thread, this);
                if (rc != 0) {
                    FLARE_LOG(FATAL) << "Fail to create sampling_thread, " << flare_error(rc);
                } else {
                    _created = true;
                    if (!registered_atfork) {
                        registered_atfork = true;
                        pthread_atfork(nullptr, nullptr, child_callback_atfork);
                    }
                }
            }

            void after_forked_as_child() {
                _created = false;
                create_sampling_thread();
            }

            void run();

            static void *sampling_thread(void *arg) {
                static_cast<sampler_collector *>(arg)->run();
                return nullptr;
            }

            static double get_cumulated_time(void *arg) {
                return static_cast<sampler_collector *>(arg)->_cumulated_time_us / 1000.0 / 1000.0;
            }

        private:
            bool _created;
            bool _stop;
            int64_t _cumulated_time_us;
            pthread_t _tid;
        };

#ifndef UNIT_TEST
        static status_gauge<double>* s_cumulated_time_var = nullptr;
        static flare::per_second<flare::status_gauge<double> >* s_sampling_thread_usage_variable = nullptr;
#endif

        void sampler_collector::run() {
#ifndef UNIT_TEST
            // NOTE:
            // * Following vars can't be created on thread's stack since this thread
            //   may be abandoned at any time after forking.
            // * They can't created inside the constructor of sampler_collector as well,
            //   which results in deadlock.
            if (s_cumulated_time_var == nullptr) {
                s_cumulated_time_var =
                    new status_gauge<double>(get_cumulated_time, this);
            }
            if (s_sampling_thread_usage_variable == nullptr) {
                s_sampling_thread_usage_variable =
                    new flare::per_second<flare::status_gauge<double> >(
                            "variable_sampler_collector_usage", s_cumulated_time_var, 10);
            }
#endif

            flare::container::link_node<variable_sampler> root;
            int consecutive_nosleep = 0;
            while (!_stop) {
                int64_t abstime = flare::get_current_time_micros();
                variable_sampler *s = this->reset();
                if (s) {
                    s->insert_before_as_list(&root);
                }
                int nremoved = 0;
                int nsampled = 0;
                for (flare::container::link_node<variable_sampler> *p = root.next(); p != &root;) {
                    // We may remove p from the list, save next first.
                    flare::container::link_node<variable_sampler> *saved_next = p->next();
                    variable_sampler *s = p->value();
                    s->_mutex.lock();
                    if (!s->_used) {
                        s->_mutex.unlock();
                        p->remove_from_list();
                        delete s;
                        ++nremoved;
                    } else {
                        s->take_sample();
                        s->_mutex.unlock();
                        ++nsampled;
                    }
                    p = saved_next;
                }
                bool slept = false;
                int64_t now = flare::get_current_time_micros();
                _cumulated_time_us += now - abstime;
                abstime += 1000000L;
                while (abstime > now) {
                    ::usleep(abstime - now);
                    slept = true;
                    now = flare::get_current_time_micros();
                }
                if (slept) {
                    consecutive_nosleep = 0;
                } else {
                    if (++consecutive_nosleep >= WARN_NOSLEEP_THRESHOLD) {
                        consecutive_nosleep = 0;
                        FLARE_LOG(WARNING) << "variable is busy at sampling for "
                                           << WARN_NOSLEEP_THRESHOLD << " seconds!";
                    }
                }
            }
        }

        variable_sampler::variable_sampler() : _used(true) {}

        variable_sampler::~variable_sampler() {}

        void variable_sampler::schedule() {
            *flare::get_leaky_singleton<sampler_collector>() << this;
        }

        void variable_sampler::destroy() {
            _mutex.lock();
            _used = false;
            _mutex.unlock();
        }

    }  // namespace metrics_detail
}  // namespace flare
