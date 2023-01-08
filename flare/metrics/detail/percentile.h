
#ifndef  FLARE_VARIABLE_DETAIL_PERCENTILE_H_
#define  FLARE_VARIABLE_DETAIL_PERCENTILE_H_

#include <string.h>                     // memset memcmp
#include <stdint.h>                     // uint32_t
#include <limits>                       // std::numeric_limits
#include <ostream>                      // std::ostream
#include <algorithm>                    // std::sort
#include <math.h>                       // ceil
#include "flare/metrics/variable_reducer.h"               // variable_reducer
#include "flare/metrics/window.h"                // window
#include "flare/metrics/detail/combiner.h"       // agent_combiner
#include "flare/metrics/detail/sampler.h"        // reducer_sampler
#include "flare/base/fast_rand.h"

namespace flare {
    namespace metrics_detail {

        // Round of expectation of a rational number |a/b| to a natural number.
        inline unsigned long round_of_expectation(unsigned long a, unsigned long b) {
            if (FLARE_UNLIKELY(b == 0)) {
                return 0;
            }
            return a / b + (flare::base::fast_rand_less_than(b) < a % b);
        }

        // Storing latencies inside a interval.
        template<size_t SAMPLE_SIZE>
        class percentile_interval {
        public:
            percentile_interval()
                    : _num_added(0), _sorted(false), _num_samples(0) {
            }

            // Get index-th sample in ascending order.
            uint32_t get_sample_at(size_t index) {
                const size_t saved_num = _num_samples;
                if (index >= saved_num) {
                    if (saved_num == 0) {
                        return 0;
                    }
                    index = saved_num - 1;
                }
                if (!_sorted) {
                    std::sort(_samples, _samples + saved_num);
                    _sorted = true;
                }
                FLARE_CHECK_EQ(saved_num, _num_samples) << "You must call get_number() on"
                                                           " a unchanging percentile_interval";
                return _samples[index];
            }

            // Add samples of another interval. This function tries to make each
            // sample in merged _samples has (approximately) equal probability to
            // remain.
            // This method is invoked when merging thread_local_percentile_samples in to
            // global_percentile_samples
            template<size_t size2>
            void merge(const percentile_interval<size2> &rhs) {
                if (rhs._num_added == 0) {
                    return;
                }
                static_assert(SAMPLE_SIZE >= size2,
                              "must merge small interval into larger one currently");
                FLARE_CHECK_EQ(rhs._num_samples, rhs._num_added);
                // Assume that the probability of each sample in |this| is a0/b0 and
                // the probability of each sample in |rhs| is a1/b1.
                // We are going to randomly pick some samples from |this| and |rhs| to
                // satisfy the constraint that each sample stands for the probability
                // of
                //     * 1 (SAMPLE_SIZE >= |b0 + b1|), which indicates that no sample
                //       has been dropped
                //     * SAMPLE_SIZE / |b0 + b1| (SAMPLE_SIZE < |b0 + b1|)
                // So we should keep |b0*SAMPLE_SIZE/(b0+b1)| from |this|
                // |b1*SAMPLE_SIZE/(b0+b1)| from |rhs|.
                if (_num_added + rhs._num_added <= SAMPLE_SIZE) {
                    // No sample should be dropped
                    FLARE_CHECK_EQ(_num_samples, _num_added)
                        << "_num_added=" << _num_added
                        << " rhs._num_added" << rhs._num_added
                        << " _num_samples=" << _num_samples
                        << " rhs._num_samples=" << rhs._num_samples
                        << " SAMPLE_SIZE=" << SAMPLE_SIZE
                        << " size2=" << size2;
                    memcpy(_samples + _num_samples, rhs._samples,
                           sizeof(_samples[0]) * rhs._num_samples);
                    _num_samples += rhs._num_samples;
                } else {
                    // |num_remain| must be less than _num_samples:
                    // if _num_added = _num_samples:
                    //    SAMPLE_SIZE / (_num_added + rhs._num_added) < 1 so that
                    //    num_remain < _num_added = _num_samples
                    // otherwise:
                    //    _num_samples = SAMPLE_SIZE;
                    //    _num_added / (_num_added + rhs._num_added) < 1 so that
                    //    num_remain < SAMPLE_SIZE = _num_added
                    size_t num_remain = round_of_expectation(
                            _num_added * SAMPLE_SIZE, _num_added + rhs._num_added);
                    FLARE_CHECK_LE(num_remain, _num_samples);
                    // Randomly drop samples of this
                    for (size_t i = _num_samples; i > num_remain; --i) {
                        _samples[flare::base::fast_rand_less_than(i)] = _samples[i - 1];
                    }
                    const size_t num_remain_from_rhs = SAMPLE_SIZE - num_remain;
                    FLARE_CHECK_LE(num_remain_from_rhs, rhs._num_samples);
                    // Have to copy data from rhs to shuffle since it's const
                    DEFINE_SMALL_ARRAY(uint32_t, tmp, rhs._num_samples, 64);
                    memcpy(tmp, rhs._samples, sizeof(uint32_t) * rhs._num_samples);
                    for (size_t i = 0; i < num_remain_from_rhs; ++i) {
                        const int index = flare::base::fast_rand_less_than(rhs._num_samples - i);
                        _samples[num_remain++] = tmp[index];
                        tmp[index] = tmp[rhs._num_samples - i - 1];
                    }
                    _num_samples = num_remain;
                    FLARE_CHECK_EQ(_num_samples, SAMPLE_SIZE);
                }
                _num_added += rhs._num_added;
            }

            // Randomly pick n samples from mutable_rhs to |this|
            template<size_t size2>
            void merge_with_expectation(percentile_interval<size2> &mutable_rhs, size_t n) {
                FLARE_CHECK(n <= mutable_rhs._num_samples);
                _num_added += mutable_rhs._num_added;
                if (_num_samples + n <= SAMPLE_SIZE && n == mutable_rhs._num_samples) {
                    memcpy(_samples + _num_samples, mutable_rhs._samples, sizeof(_samples[0]) * n);
                    _num_samples += n;
                    return;
                }
                for (size_t i = 0; i < n; ++i) {
                    size_t index = flare::base::fast_rand_less_than(mutable_rhs._num_samples - i);
                    if (_num_samples < SAMPLE_SIZE) {
                        _samples[_num_samples++] = mutable_rhs._samples[index];
                    } else {
                        _samples[flare::base::fast_rand_less_than(_num_samples)]
                                = mutable_rhs._samples[index];
                    }
                    std::swap(mutable_rhs._samples[index],
                              mutable_rhs._samples[mutable_rhs._num_samples - i - 1]);
                }
            }

            // Add an unsigned 32-bit latency (what percentile actually accepts) to a
            // non-full interval, which is invoked by percentile::operator<< to add a
            // sample into the thread_local_percentile_samples.
            // Returns true if the input was stored.
            bool add32(uint32_t x) {
                if (FLARE_UNLIKELY(_num_samples >= SAMPLE_SIZE)) {
                    FLARE_LOG(ERROR) << "This interval was full";
                    return false;
                }
                ++_num_added;
                _samples[_num_samples++] = x;
                return true;
            }

            // Add a signed latency.
            bool add64(int64_t x) {
                if (x >= 0) {
                    return add32((uint32_t) x);
                }
                return false;
            }

            // Remove all samples inside.
            void clear() {
                _num_added = 0;
                _sorted = false;
                _num_samples = 0;
            }

            // True if no more room for new samples.
            bool full() const { return _num_samples == SAMPLE_SIZE; }

            // True if there's no samples.
            bool empty() const { return !_num_samples; }

            // #samples ever added by calling add*()
            uint32_t added_count() const { return _num_added; }

            // #samples stored.
            uint32_t sample_count() const { return _num_samples; }

            // For debuggin.
            void describe(std::ostream &os) const {
                os << "(num_added=" << added_count() << ")[";
                for (size_t j = 0; j < _num_samples; ++j) {
                    os << ' ' << _samples[j];
                }
                os << " ]";
            }

            // True if two percentile_interval are exactly same, namely same # of added and
            // same samples, mainly for debuggin.
            bool operator==(const percentile_interval &rhs) const {
                return (_num_added == rhs._num_added &&
                        _num_samples == rhs._num_samples &&
                        memcmp(_samples, rhs._samples, _num_samples * sizeof(uint32_t)) == 0);
            }

        private:

            template<size_t size2> friend
            class percentile_interval;

            static_assert(SAMPLE_SIZE <= 65536, "SAMPLE_SIZE must be 6bit");

            uint32_t _num_added;
            bool _sorted;
            uint16_t _num_samples;
            uint32_t _samples[SAMPLE_SIZE];
        };

        static const size_t NUM_INTERVALS = 32;

        // This declartion is a must for gcc 3.4
        class add_latency;

        // Group of PercentileIntervals.
        template<size_t SAMPLE_SIZE_IN>
        class percentile_samples {
        public:
            friend class add_latency;

            static const size_t SAMPLE_SIZE = SAMPLE_SIZE_IN;

            percentile_samples() {
                memset(this, 0, sizeof(*this));
            }

            ~percentile_samples() {
                for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                    if (_intervals[i]) {
                        delete _intervals[i];
                    }
                }
            }

            // Copy-construct from another percentile_samples.
            // Copy/assigning happen at per-second scale. should be OK.
            percentile_samples(const percentile_samples &rhs) {
                _num_added = rhs._num_added;
                for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                    if (rhs._intervals[i] && !rhs._intervals[i]->empty()) {
                        _intervals[i] = new percentile_interval<SAMPLE_SIZE>(*rhs._intervals[i]);
                    } else {
                        _intervals[i] = nullptr;
                    }
                }
            }

            // Assign from another percentile_samples.
            // Notice that we keep empty _intervals to avoid future allocations.
            void operator=(const percentile_samples &rhs) {
                _num_added = rhs._num_added;
                for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                    if (rhs._intervals[i] && !rhs._intervals[i]->empty()) {
                        get_interval_at(i) = *rhs._intervals[i];
                    } else if (_intervals[i]) {
                        _intervals[i]->clear();
                    }
                }
            }

            // Get the `ratio'-ile value. E.g. 0.99 means 99%-ile value.
            // Since we store samples in different intervals internally. We first
            // address the interval by multiplying ratio with _num_added, then
            // find the sample inside the interval. We've tested an alternative
            // method that store all samples together w/o any intervals (or in another
            // word, only one interval), the method is much simpler but is not as
            // stable as current impl. CDF plotted by the method changes dramatically
            // from seconds to seconds. It seems that separating intervals probably
            // keep more long-tail values.
            uint32_t get_number(double ratio) {
                size_t n = (size_t) ceil(ratio * _num_added);
                if (n > _num_added) {
                    n = _num_added;
                } else if (n == 0) {
                    return 0;
                }
                for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                    if (_intervals[i] == nullptr) {
                        continue;
                    }
                    percentile_interval<SAMPLE_SIZE> &invl = *_intervals[i];
                    if (n <= invl.added_count()) {
                        size_t sample_n = n * invl.sample_count() / invl.added_count();
                        size_t sample_index = (sample_n ? sample_n - 1 : 0);
                        return invl.get_sample_at(sample_index);
                    }
                    n -= invl.added_count();
                }
                FLARE_CHECK(false) << "Can't reach here";
                return std::numeric_limits<uint32_t>::max();
            }

            // Add samples in another percentile_samples.
            template<size_t size2>
            void merge(const percentile_samples<size2> &rhs) {
                _num_added += rhs._num_added;
                for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                    if (rhs._intervals[i] && !rhs._intervals[i]->empty()) {
                        get_interval_at(i).merge(*rhs._intervals[i]);
                    }
                }
            }

            // Combine multiple into a single percentile_samples
            template<typename Iterator>
            void combine_of(const Iterator &begin, const Iterator &end) {
                if (_num_added) {
                    // Very unlikely
                    for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                        if (_intervals[i]) {
                            _intervals[i]->clear();
                        }
                    }
                    _num_added = 0;
                }

                for (Iterator iter = begin; iter != end; ++iter) {
                    _num_added += iter->_num_added;
                }

                // Calculate probabilities for each interval
                for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                    size_t total = 0;
                    size_t total_sample = 0;
                    for (Iterator iter = begin; iter != end; ++iter) {
                        if (iter->_intervals[i]) {
                            total += iter->_intervals[i]->added_count();
                            total_sample += iter->_intervals[i]->sample_count();
                        }
                    }
                    if (total == 0) {
                        // Empty interval
                        continue;
                    }


                    // Consider that sub interval took |a| samples out of |b| totally,
                    // each sample won the probability of |a/b| according to the
                    // algorithm of add32(), now we should pick some samples into the
                    // combined interval that satisfied each sample has the
                    // probability of |SAMPLE_SIZE/total|, so each sample has the
                    // probability of |(SAMPLE_SIZE*b)/(a*total) to remain and the
                    // expected number of samples in this interval is
                    // |(SAMPLE_SIZE*b)/total|
                    for (Iterator iter = begin; iter != end; ++iter) {
                        if (!iter->_intervals[i] || iter->_intervals[i]->empty()) {
                            continue;
                        }
                        typename flare::add_reference<decltype(*(iter->_intervals[i]))>::type
                                invl = *(iter->_intervals[i]);
                        if (total <= SAMPLE_SIZE) {
                            get_interval_at(i).merge_with_expectation(
                                    invl, invl.sample_count());
                            continue;
                        }
                        // Each
                        const size_t b = invl.added_count();
                        const size_t remain = std::min(
                                round_of_expectation(b * SAMPLE_SIZE, total),
                                (size_t) invl.sample_count());
                        get_interval_at(i).merge_with_expectation(invl, remain);
                    }
                }
            }

            // For debuggin.
            void describe(std::ostream &os) const {
                os << this << "{num_added=" << _num_added;
                for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                    if (_intervals[i] && !_intervals[i]->empty()) {
                        os << " interval[" << i << "]=";
                        _intervals[i]->describe(os);
                    }
                }
                os << '}';
            }

            // True if intervals of two percentile_samples are exactly same.
            bool operator==(const percentile_samples &rhs) const {
                for (size_t i = 0; i < NUM_INTERVALS; ++i) {
                    if (_intervals != rhs._intervals[i]) {
                        return false;
                    }
                }
                return true;
            }

        private:

            template<size_t size1> friend
            class percentile_samples;

            // Get/create interval on-demand.
            percentile_interval<SAMPLE_SIZE> &get_interval_at(size_t index) {
                if (_intervals[index] == nullptr) {
                    _intervals[index] = new percentile_interval<SAMPLE_SIZE>;
                }
                return *_intervals[index];
            }

            // sum of _num_added of all intervals. we update this value after any
            // changes to intervals inside to make it O(1)-time accessible.
            size_t _num_added;
            percentile_interval<SAMPLE_SIZE> *_intervals[NUM_INTERVALS];
        };

        template<size_t sz> const size_t percentile_samples<sz>::SAMPLE_SIZE;

        template<size_t size>
        std::ostream &operator<<(std::ostream &os, const percentile_interval<size> &p) {
            p.describe(os);
            return os;
        }

        template<size_t size>
        std::ostream &operator<<(std::ostream &os, const percentile_samples<size> &p) {
            p.describe(os);
            return os;
        }

        // NOTE: we intentionally minus 2 uint32_t from sample-size to make the struct
        // size be power of 2 and more friendly to memory allocators.
        typedef percentile_samples<254> global_percentile_samples;
        typedef percentile_samples<30> thread_local_percentile_samples;

        // A specialized reducer for finding the percentile of latencies.
        // NOTE: DON'T use it directly, use LatencyRecorder instead.
        class percentile {
        public:
            struct add_percentile_samples {
                template<size_t size1, size_t size2>
                void operator()(percentile_samples<size1> &b1,
                                const percentile_samples<size2> &b2) const {
                    b1.merge(b2);
                }
            };

            typedef global_percentile_samples value_type;
            typedef reducer_sampler <percentile,
            global_percentile_samples,
            add_percentile_samples, void_op> sampler_type;
            typedef agent_combiner<global_percentile_samples,
                    thread_local_percentile_samples,
                    add_percentile_samples> combiner_type;
            typedef combiner_type::Agent agent_type;

            percentile();

            ~percentile();

            add_percentile_samples op() const { return add_percentile_samples(); }

            void_op inv_op() const { return void_op(); }

            // The sampler for windows over percentile.
            sampler_type *get_sampler() {
                if (nullptr == _sampler) {
                    _sampler = new sampler_type(this);
                    _sampler->schedule();
                }
                return _sampler;
            }

            value_type reset();

            value_type get_value() const;

            percentile &operator<<(int64_t latency);

            bool valid() const { return _combiner != nullptr && _combiner->valid(); }

            // This name is useful for warning negative latencies in operator<<
            void set_debug_name(const std::string_view &name) {
                _debug_name.assign(name.data(), name.size());
            }

        private:
            FLARE_DISALLOW_COPY_AND_ASSIGN(percentile);

            combiner_type *_combiner;
            sampler_type *_sampler;
            std::string _debug_name;
        };

    }  // namespace metrics_detail
}  // namespace flare

#endif  // FLARE_VARIABLE_DETAIL_PERCENTILE_H_
