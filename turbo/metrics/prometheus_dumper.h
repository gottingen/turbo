
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_PROMETHUES_DUMPER_H_
#define TURBO_PROMETHUES_DUMPER_H_

#include "turbo/metrics/variable_base.h"
#include "turbo/metrics/dumper.h"
#include "turbo/metrics/cache_metric.h"
#include "turbo/io/cord_buf.h"
#include "turbo/log/logging.h"

namespace turbo {

    class prometheus_dumper : public metrics_dumper {
    public:
        explicit prometheus_dumper(cord_buf_builder *buf) : _buf(buf) {
            TURBO_CHECK(_buf);
        }

        bool dump(const cache_metrics &metric, const turbo::time_point *tp) override;

        static std::string dump_to_string(const cache_metrics &metric, const turbo::time_point *tp);
    private:

        cord_buf_builder *_buf;
    };
}  // namespace turbo

#endif  // TURBO_PROMETHUES_DUMPER_H_
