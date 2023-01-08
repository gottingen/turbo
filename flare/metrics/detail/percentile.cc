
#include "flare/metrics/detail/percentile.h"
#include "flare/log/logging.h"

namespace flare {
    namespace metrics_detail {

        inline uint32_t ones32(uint32_t x) {
            /* 32-bit recursive reduction using SWAR...
             * but first step is mapping 2-bit values
             * into sum of 2 1-bit values in sneaky way
             */
            x -= ((x >> 1) & 0x55555555);
            x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
            x = (((x >> 4) + x) & 0x0f0f0f0f);
            x += (x >> 8);
            x += (x >> 16);
            return (x & 0x0000003f);
        }

        inline uint32_t log2(uint32_t x) {
            int y = (x & (x - 1));
            y |= -y;
            y >>= 31;
            x |= (x >> 1);
            x |= (x >> 2);
            x |= (x >> 4);
            x |= (x >> 8);
            x |= (x >> 16);
            return (ones32(x) - 1 - y);
        }

        inline size_t get_interval_index(int64_t & x) {
            if (x <= 2) {
                return 0;
            } else if (x > std::numeric_limits<uint32_t>::max()) {
                x = std::numeric_limits<uint32_t>::max();
                return 31;
            } else {
                return log2(x) - 1;
            }
        }

        class add_latency {
        public:
            add_latency(int64_t latency) : _latency(latency) {}

            void operator()(GlobalValue<percentile::combiner_type> &global_value,
                            thread_local_percentile_samples &local_value) const {
                // Copy to latency since get_interval_index may change input.
                int64_t latency = _latency;
                const size_t index = get_interval_index(latency);
                percentile_interval <thread_local_percentile_samples::SAMPLE_SIZE> &
                        interval = local_value.get_interval_at(index);
                if (interval.full()) {
                    global_percentile_samples *g = global_value.lock();
                    g->get_interval_at(index).merge(interval);
                    g->_num_added += interval.added_count();
                    global_value.unlock();
                    local_value._num_added -= interval.added_count();
                    interval.clear();
                }
                interval.add64(latency);
                ++local_value._num_added;
            }

        private:
            int64_t _latency;
        };

        percentile::percentile() : _combiner(nullptr), _sampler(nullptr) {
            _combiner = new combiner_type;
        }

        percentile::~percentile() {
            // Have to destroy sampler first to avoid the race between destruction and
            // sampler
            if (_sampler != nullptr) {
                _sampler->destroy();
                _sampler = nullptr;
            }
            delete _combiner;
        }

        percentile::value_type percentile::reset() {
            return _combiner->reset_all_agents();
        }

        percentile::value_type percentile::get_value() const {
            return _combiner->combine_agents();
        }

        percentile &percentile::operator<<(int64_t latency) {
            agent_type *agent = _combiner->get_or_create_tls_agent();
            if (FLARE_UNLIKELY(!agent)) {
                FLARE_LOG(FATAL) << "Fail to create agent";
                return *this;
            }
            if (latency < 0) {
                // we don't check overflow(of uint32) in percentile because the
                // overflowed value which is included in last range does not affect
                // overall distribution of other values too much.
                if (!_debug_name.empty()) {
                    FLARE_LOG(WARNING) << "Input=" << latency << " to `" << _debug_name
                                       << "' is negative, drop";
                } else {
                    FLARE_LOG(WARNING) << "Input=" << latency << " to percentile("
                                       << (void *) this << ") is negative, drop";
                }
                return *this;
            }
            agent->merge_global(add_latency(latency));
            return *this;
        }

    }  // namespace metrics_detail
}  // namespace flare
