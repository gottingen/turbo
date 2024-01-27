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


#include <map>
#include <gflags/gflags.h>
#include "turbo/system/threading.h"
#include "turbo/var/passive_status.h"
#include "turbo/var/collector.h"
#include "turbo/flags/flag.h"

TURBO_FLAG(int32_t, var_collector_max_pending_samples, 1000,
           "Destroy unprocessed samples when they're too many");
TURBO_FLAG(int32_t, var_collector_expected_per_second, 1000,
           "Expected number of samples to be collected per second");

namespace turbo {


    // CAUTION: Don't change this value unless you know exactly what it means.
    static const int64_t COLLECTOR_GRAB_INTERVAL_US = 100000L; // 100ms

    static_assert(!(COLLECTOR_SAMPLING_BASE & (COLLECTOR_SAMPLING_BASE - 1)),
                  "must be power of 2");

    // Combine two circular linked list into one.
    struct CombineCollected {
        void operator()(Collected *&s1, Collected *s2) const {
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

    // A thread and a special var to collect samples submitted.
    class Collector : public turbo::Reducer<Collected *, CombineCollected> {
    public:
        static Collector *get_instance() {
            static Collector instance;
            return &instance;
        }

        Collector();

        ~Collector();

        int64_t last_active_cpuwide_us() const { return _last_active_cpuwide_us; }

        void wakeup_grab_thread();

    private:
        // The thread for collecting TLS submissions.
        void grab_thread();

        // The thread for calling user's callbacks.
        void dump_thread();

        void update_speed_limit(CollectorSpeedLimit *speed_limit,
                                size_t *last_ngrab, size_t cur_ngrab,
                                int64_t interval_us);

        static void *run_grab_thread(void *arg) {
            PlatformThread::set_name("var_collector_grabber");
            static_cast<Collector *>(arg)->grab_thread();
            return nullptr;
        }

        static void *run_dump_thread(void *arg) {
            PlatformThread::set_name("var_collector_dumper");
            static_cast<Collector *>(arg)->dump_thread();
            return nullptr;
        }

        static int64_t get_pending_count(void *arg) {
            Collector *d = static_cast<Collector *>(arg);
            return d->_ngrab - d->_ndump - d->_ndrop;
        }

    private:
        // periodically modified by grab_thread, accessed by every submit.
        // Make sure that this cacheline does not include frequently modified field.
        int64_t _last_active_cpuwide_us;

        bool _created;      // Mark validness of _grab_thread.
        bool _stop;         // Set to true in dtor.
        pthread_t _grab_thread;     // For joining.
        pthread_t _dump_thread;
        int64_t _ngrab TURBO_CACHE_LINE_ALIGNED;
        int64_t _ndrop;
        int64_t _ndump;
        std::mutex _dump_thread_mutex;
        std::condition_variable _dump_thread_cond;
        turbo::intrusive_list_node _dump_root;
        std::mutex _sleep_mutex;
        std::condition_variable _sleep_cond;
    };

    Collector::Collector()
            : _last_active_cpuwide_us(turbo::get_current_time_micros()), _created(false), _stop(false), _grab_thread(0),
              _dump_thread(0), _ngrab(0), _ndrop(0), _ndump(0) {
        int rc = pthread_create(&_grab_thread, nullptr, run_grab_thread, this);
        TURBO_RAW_LOG(INFO, "Collector created");
        if (rc != 0) {
            TURBO_RAW_LOG(FATAL, "Fail to create Collector, %s", terror());
        } else {
            _created = true;
        }
    }

    Collector::~Collector() {
        if (_created) {
            _stop = true;
            pthread_join(_grab_thread, nullptr);
            _created = false;
        }
    }

    template<typename T>
    static T deref_value(void *arg) {
        return *(T *) arg;
    }

    // for limiting samples returning nullptr in speed_limit()
    static CollectorSpeedLimit g_null_speed_limit;

    void Collector::grab_thread() {
        _last_active_cpuwide_us = turbo::get_current_time_micros();
        int64_t last_before_update_sl = _last_active_cpuwide_us;

        // This is the thread for collecting TLS submissions. User's callbacks are
        // called inside the separate _dump_thread to prevent a slow callback
        // (caused by busy disk generally) from blocking collecting code too long
        // that pending requests may explode memory.
        if(0 != pthread_create(&_dump_thread, nullptr, run_dump_thread, this)) {
            TURBO_RAW_LOG(FATAL, "Fail to create dump thread, %s", terror());
        }

        // vars
        turbo::PassiveStatus<int64_t> pending_sampled_data(
                "var_collector_pending_samples", get_pending_count, this);
        double busy_seconds = 0;
        turbo::PassiveStatus<double> busy_seconds_var(deref_value<double>, &busy_seconds);
        turbo::PerSecond<turbo::PassiveStatus<double>> busy_seconds_second(
                "var_collector_grab_thread_usage", &busy_seconds_var);

        turbo::PassiveStatus<int64_t> ngrab_var(deref_value<int64_t>, &_ngrab);
        turbo::PerSecond<turbo::PassiveStatus<int64_t>> ngrab_second(
                "var_collector_grab_second", &ngrab_var);

        // Maps for calculating speed limit.
        typedef std::map<CollectorSpeedLimit *, size_t> GrapMap;
        GrapMap last_ngrab_map;
        GrapMap ngrab_map;
        // Map for group samples by preprocessors.
        typedef std::map<CollectorPreprocessor *, std::vector<Collected *> >
                PreprocessorMap;
        PreprocessorMap prep_map;

        // The main loop.
        while (!_stop) {
            const int64_t abstime = _last_active_cpuwide_us + COLLECTOR_GRAB_INTERVAL_US;

            // Clear and reuse vectors in prep_map, don't clear prep_map directly.
            for (PreprocessorMap::iterator it = prep_map.begin(); it != prep_map.end();
                 ++it) {
                it->second.clear();
            }

            // Collect TLS submissions and give them to dump_thread.
            Collected *head = this->reset();
            if (head) {
                turbo::intrusive_list_node anchor;
                head->insert_before_as_list(&anchor);
                head = nullptr;

                // Group samples by preprocessors.
                for (Collected *p = static_cast<Collected *>(anchor.next); p != &anchor;) {
                    auto *saved_next = static_cast<Collected *>(p->next);
                    p->remove_from_list();
                    CollectorPreprocessor *prep = p->preprocessor();
                    prep_map[prep].push_back(p);
                    p = saved_next;
                }
                // Iterate prep_map
                intrusive_list_node root;
                for (PreprocessorMap::iterator it = prep_map.begin();
                     it != prep_map.end(); ++it) {
                    std::vector<Collected *> &list = it->second;
                    if (it->second.empty()) {
                        // don't call preprocessor when there's no samples.
                        continue;
                    }
                    if (it->first != nullptr) {
                        it->first->process(list);
                    }
                    for (size_t i = 0; i < list.size(); ++i) {
                        Collected *p = list[i];
                        CollectorSpeedLimit *speed_limit = p->speed_limit();
                        if (speed_limit == nullptr) {
                            ++ngrab_map[&g_null_speed_limit];
                        } else {
                            // Add up the samples of certain type.
                            ++ngrab_map[speed_limit];
                        }
                        ++_ngrab;
                        if (_ngrab >= _ndrop + _ndump +
                                      get_flag(FLAGS_var_collector_max_pending_samples)) {
                            ++_ndrop;
                            p->destroy();
                        } else {
                            p->insert_before(&root);
                        }
                    }
                }
                // Give the samples to dump_thread
                if (root.next != &root) {  // non empty
                    auto *head2 = static_cast<Collected *>(root.next);
                    root.remove_from_list();
                    std::unique_lock l(_dump_thread_mutex);
                    head2->insert_before_as_list(&_dump_root);
                    _dump_thread_cond.notify_one();
                }
            }
            int64_t now = turbo::get_current_time_micros();
            int64_t interval = now - last_before_update_sl;
            last_before_update_sl = now;
            for (GrapMap::iterator it = ngrab_map.begin();
                 it != ngrab_map.end(); ++it) {
                update_speed_limit(it->first, &last_ngrab_map[it->first],
                                   it->second, interval);
            }

            now = turbo::get_current_time_micros();
            // calcuate thread usage.
            busy_seconds += (now - _last_active_cpuwide_us) / 1000000.0;
            _last_active_cpuwide_us = now;

            // sleep for the next round.
            if (!_stop && abstime > now) {
                auto deadline = turbo::Time::from_microseconds(abstime).to_chrono_time();
                std::unique_lock l(_sleep_mutex);
                _sleep_cond.wait_until(l, deadline);
            }
            _last_active_cpuwide_us = turbo::get_current_time_micros();
        }
        // make sure _stop is true, we may have other reasons to quit above loop
        {
            std::unique_lock l(_dump_thread_mutex);
            _stop = true;
            _dump_thread_cond.notify_one();
        }
        pthread_join(_dump_thread, nullptr);
    }

    void Collector::wakeup_grab_thread() {
        std::unique_lock l(_sleep_mutex);
        _sleep_cond.notify_one();
    }

    void Collector::update_speed_limit(CollectorSpeedLimit *sl,
                                       size_t *last_ngrab, size_t cur_ngrab,
                                       int64_t interval_us) {
        const size_t round_ngrab = cur_ngrab - *last_ngrab;
        if (round_ngrab == 0) {
            return;
        }
        *last_ngrab = cur_ngrab;
        if (interval_us < 0) {
            interval_us = 0;
        }
        size_t new_sampling_range = 0;
        const size_t old_sampling_range = sl->sampling_range;
        if (!sl->ever_grabbed) {
            if (sl->first_sample_real_us) {
                interval_us = turbo::get_current_time_micros() - sl->first_sample_real_us;
                if (interval_us < 0) {
                    interval_us = 0;
                }
            } else {
            }
            new_sampling_range = get_flag(FLAGS_var_collector_expected_per_second)
                                 * interval_us * COLLECTOR_SAMPLING_BASE / (1000000L * round_ngrab);
        } else {
            // NOTE: the multiplications are unlikely to overflow.
            new_sampling_range = get_flag(FLAGS_var_collector_expected_per_second)
                                 * interval_us * old_sampling_range / (1000000L * round_ngrab);
            // Don't grow or shrink too fast.
            if (interval_us < 1000000L) {
                new_sampling_range =
                        (new_sampling_range * interval_us +
                         old_sampling_range * (1000000L - interval_us)) / 1000000L;
            }
        }
        // Make sure new value is sane.
        if (new_sampling_range == 0) {
            new_sampling_range = 1;
        } else if (new_sampling_range > COLLECTOR_SAMPLING_BASE) {
            new_sampling_range = COLLECTOR_SAMPLING_BASE;
        }

        // NOTE: don't update unmodified fields in sl to avoid meaningless
        // flushing of the cacheline.
        if (new_sampling_range != old_sampling_range) {
            sl->sampling_range = new_sampling_range;
        }
        if (!sl->ever_grabbed) {
            sl->ever_grabbed = true;
        }
    }

    size_t is_collectable_before_first_time_grabbed(CollectorSpeedLimit *sl) {
        if (!sl->ever_grabbed) {
            int before_add = sl->count_before_grabbed.fetch_add(
                    1, std::memory_order_relaxed);
            if (before_add == 0) {
                sl->first_sample_real_us = turbo::get_current_time_micros();
            } else if (before_add >= get_flag(FLAGS_var_collector_expected_per_second)) {
                Collector::get_instance()->wakeup_grab_thread();
            }
        }
        return sl->sampling_range;
    }

    // Call user's callbacks in this thread.
    void Collector::dump_thread() {
        int64_t last_ns = turbo::get_current_time_micros();

        // vars
        double busy_seconds = 0;
        turbo::PassiveStatus<double> busy_seconds_var(deref_value<double>, &busy_seconds);
        turbo::PerSecond<turbo::PassiveStatus<double>> busy_seconds_second(
                "var_collector_dump_thread_usage", &busy_seconds_var);

        turbo::PassiveStatus<int64_t> ndumped_var(deref_value<int64_t>, &_ndump);
        turbo::PerSecond<turbo::PassiveStatus<int64_t>> ndumped_second(
                "var::collector_dump_second", &ndumped_var);

        intrusive_list_node root;
        size_t round = 0;

        // The main loop
        while (!_stop) {
            ++round;
            TURBO_RAW_LOG(INFO, "Collector dump thread round %zu", round);
            // Get new samples set by grab_thread.
            Collected *newhead = nullptr;
            {
                std::unique_lock l(_dump_thread_mutex);
                while (!_stop && _dump_root.next == &_dump_root) {
                    const int64_t now_ns = turbo::get_current_time_micros();
                    busy_seconds += (now_ns - last_ns) / 1000000000.0;
                    _dump_thread_cond.wait(l);
                    last_ns = turbo::get_current_time_micros();
                }
                if (_stop) {
                    break;
                }
                newhead = static_cast<Collected *>(_dump_root.next);
                _dump_root.remove_from_list();
            }
            TURBO_ASSERT(newhead != &_dump_root);
            newhead->insert_before_as_list(&root);


            for (Collected *p = static_cast<Collected *>(root.next); !_stop && p != &root;) {
                // We remove p from the list, save next first.
                auto *saved_next = static_cast<Collected *>(p->next);
                p->remove_from_list();
                p->dump_and_destroy(round);
                ++_ndump;
                p = saved_next;
            }
        }
    }

    void Collected::submit(int64_t cpuwide_us) {
        Collector *d = Collector::get_instance();
        if (cpuwide_us < d->last_active_cpuwide_us() + COLLECTOR_GRAB_INTERVAL_US * 2) {
            *d << this;
        } else {
            destroy();
        }
    }

    static double get_sampling_ratio(void *arg) {
        return ((const CollectorSpeedLimit *) arg)->sampling_range /
               (double) COLLECTOR_SAMPLING_BASE;
    }

    DisplaySamplingRatio::DisplaySamplingRatio(const char *name,
                                               const CollectorSpeedLimit *sl)
            : _var(name, get_sampling_ratio, (void *) sl) {
    }

}  // namespace turbo