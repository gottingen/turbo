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

#include "turbo/var/operators.h"
#include "turbo/var/internal/sampler.h"
#include "turbo/var/passive_status.h"
#include "turbo/var/window.h"
#include "turbo/times/time.h"
#include "turbo/flags/flag.h"

TURBO_FLAG(turbo::Duration, var_sampler_thread_start_delay, turbo::Duration::milliseconds(10),
           "var sampler thread start delay us");
TURBO_FLAG(bool, var_enable_sampling, true, "is enable var sampling");

namespace turbo::var_internal {

    const int WARN_NOSLEEP_THRESHOLD = 2;

    struct CombineSampler {
        void operator()(Sampler *&s1, Sampler *s2) const {
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
    // creation overhead of Window<> is negliable.
    // The trick is to use Reducer<Sampler*, CombineSampler>. Each Sampler is
    // doubly linked, thus we can reduce multiple Samplers into one cicurlarly
    // doubly linked list, and multiple lists into larger lists. We create a
    // dedicated thread to periodically get_value() which is just the combined
    // list of Samplers. Waking through the list and call take_sample().
    // If a Sampler needs to be deleted, we just mark it as unused and the
    // deletion is taken place in the thread as well.
    class SamplerCollector {
    public:
        SamplerCollector()
                : _created(false), _stop(false), _cumulated_time_us(0) {
            create_sampling_thread();
        }

        ~SamplerCollector() {
            if (_created) {
                _stop = true;
                pthread_join(_tid, nullptr);
                _created = false;
            }
        }

        static SamplerCollector *get_instance() {
            static SamplerCollector instance;
            return &instance;
        }

        void add_sampler(Sampler *sampler) {
            //std::lock_guard<std::mutex> lock(_mutex);
            //added_list.push_back(*sampler);
        }

    private:
        // Support for fork:
        // * The singleton can be null before forking, the child callback will not
        //   be registered.
        // * If the singleton is not null before forking, the child callback will
        //   be registered and the sampling thread will be re-created.
        // * A forked program can be forked again.

        static void child_callback_atfork() {
            SamplerCollector::get_instance()->after_forked_as_child();
        }

        void create_sampling_thread() {
            const int rc = pthread_create(&_tid, nullptr, sampling_thread, this);
            if (rc != 0) {
                TURBO_RAW_LOG(FATAL, "Fail to create sampling_thread, %s", terror(rc));
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
            PlatformThread::set_name("var_sampler");
            static_cast<SamplerCollector *>(arg)->run();
            return nullptr;
        }

        static double get_cumulated_time(void *arg) {
            return static_cast<SamplerCollector *>(arg)->_cumulated_time_us / 1000.0 / 1000.0;
        }

    private:
        bool _created;
        bool _stop;
        int64_t _cumulated_time_us;
        pthread_t _tid;
        std::mutex _mutex;
        turbo::intrusive_list<Sampler> added_list TURBO_GUARDED_BY(_mutex);
        turbo::intrusive_list<Sampler> doing_list;
    };

#ifndef UNIT_TEST

    static PassiveStatus<double> *s_cumulated_time_var = nullptr;
    static turbo::PerSecond<turbo::PassiveStatus<double>> *s_sampling_thread_usage_bvar = nullptr;

#endif


    void SamplerCollector::run() {
        turbo::sleep_for(get_flag(FLAGS_var_sampler_thread_start_delay));

#ifndef UNIT_TEST
        // NOTE:
        // * Following vars can't be created on thread's stack since this thread
        //   may be abandoned at any time after forking.
        // * They can't created inside the constructor of SamplerCollector as well,
        //   which results in deadlock.
        /*
        if (s_cumulated_time_var == nullptr) {
            s_cumulated_time_var =
                    new turbo::PassiveStatus<double>(get_cumulated_time, this);
        }
        if (s_sampling_thread_usage_bvar == nullptr) {
            s_sampling_thread_usage_bvar =
                    new turbo::PerSecond<turbo::PassiveStatus<double>>(
                            "var_sampler_collector_usage", s_cumulated_time_var, 10);
        }
        */
#endif
        int consecutive_nosleep = 0;
        while (!_stop) {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                doing_list.splice(doing_list.end(), added_list);
            }
            int64_t abstime = turbo::get_current_time_micros();
            for (auto it = doing_list.begin(); it != doing_list.end();) {
                auto sit = it++;
                sit->_mutex.lock();
                if (!sit->_used) {
                    doing_list.erase(sit);
                    sit->_mutex.unlock();
                    delete sit.node_ptr();
                } else {
                    sit->take_sample();
                    sit->_mutex.unlock();
                }
            }
            bool slept = false;
            int64_t now = turbo::get_current_time_micros();
            _cumulated_time_us += now - abstime;
            abstime += 1000000L;
            while (abstime > now) {
                ::usleep(abstime - now);
                slept = true;
                now = turbo::get_current_time_micros();
            }
            if (slept) {
                consecutive_nosleep = 0;
            } else {
                if (++consecutive_nosleep >= WARN_NOSLEEP_THRESHOLD) {
                    consecutive_nosleep = 0;
                    TURBO_RAW_LOG(WARNING, "var is busy at sampling for %d seconds!",
                                  WARN_NOSLEEP_THRESHOLD);
                }
            }
        }
    }

    Sampler::Sampler() : _used(true) {}

    Sampler::~Sampler() {}


    void Sampler::schedule() {
        if (get_flag(FLAGS_var_enable_sampling)) {
            SamplerCollector::get_instance()->add_sampler(this);
        }
    }

    void Sampler::destroy() {
        _mutex.lock();
        _used = false;
        _mutex.unlock();
    }

}  // namespace namespace turbo::var_internal
