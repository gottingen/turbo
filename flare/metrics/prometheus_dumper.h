
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_PROMETHUES_DUMPER_H_
#define FLARE_PROMETHUES_DUMPER_H_

#include "flare/metrics/variable_base.h"
#include "flare/metrics/dumper.h"
#include "flare/metrics/cache_metric.h"
#include "flare/io/cord_buf.h"
#include "flare/log/logging.h"

namespace flare {

    class prometheus_dumper : public metrics_dumper {
    public:
        explicit prometheus_dumper(cord_buf_builder *buf) : _buf(buf) {
            FLARE_CHECK(_buf);
        }

        bool dump(const cache_metrics &metric, const flare::time_point *tp) override;

        static std::string dump_to_string(const cache_metrics &metric, const flare::time_point *tp);
    private:

        cord_buf_builder *_buf;
    };
}  // namespace flare

#endif  // FLARE_PROMETHUES_DUMPER_H_
