
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_METRICS_DUMPER_H_
#define FLARE_METRICS_DUMPER_H_

#include <string>
#include "flare/times/time.h"

namespace flare {

    // Bitwise masks of displayable targets
    enum display_filter {
        DISPLAY_NON = 0,
        DISPLAY_ON_HTML = 1,
        DISPLAY_ON_PLAIN_TEXT = 2,
        DISPLAY_ON_ALL = 3,
        DISPLAY_ON_METRICS = 4,
    };

    // Implement this class to write variables into different places.
    // If dump() returns false, variable_base::dump_exposed() stops and returns -1.
    class variable_dumper {
    public:
        virtual ~variable_dumper() {}

        virtual bool dump(const std::string &name,
                          const std::string_view &description) = 0;
    };

    class metrics_dumper {
    public:
        virtual ~metrics_dumper() {}

        virtual bool dump(const cache_metrics &metric, const flare::time_point *tp) = 0;
    };

    // Options for variable_base::dump_exposed().
    struct metrics_dump_options {
        // The ? in wildcards. Wildcards in URL need to use another character
        // because ? is reserved.
        char question_mark = '?';
        // Name matched by these wildcards (or exact names) are kept.
        std::string white_wildcards;

        // Name matched by these wildcards (or exact names) are skipped.
        std::string black_wildcards;

        flare::time_point *dump_time{nullptr};
    };

    // Options for variable_base::dump_exposed().
    struct variable_dump_options {
        // Constructed with default options.
        variable_dump_options();

        // If this is true, string-type values will be quoted.
        bool quote_string;

        // The ? in wildcards. Wildcards in URL need to use another character
        // because ? is reserved.
        char question_mark;

        // Dump variables with matched display_filter
        display_filter filter;

        // Name matched by these wildcards (or exact names) are kept.
        std::string white_wildcards;

        // Name matched by these wildcards (or exact names) are skipped.
        std::string black_wildcards;
    };
}  // namespace flare

#endif  // FLARE_METRICS_DUMPER_H_
