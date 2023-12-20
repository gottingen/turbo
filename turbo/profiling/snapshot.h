// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//
// Created by jeff on 23-12-20.
//

#ifndef TURBO_PROFILING_SNAPSHOT_H_
#define TURBO_PROFILING_SNAPSHOT_H_

#include <variant>
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include "turbo/times/clock.h"

namespace turbo {

    enum class VariableType : uint8_t {
        VT_PLAIN_STRING = 0,
        VT_COUNTER = 1,
        VT_GAUGE_SCALAR = 1 << 1,
        VT_HISTOGRAM = 1 << 2,
        VT_PLAIN_INT = 1 << 3,
        VT_OBJECT = 1 << 4,
        VT_PROMETHEUS = VT_COUNTER | VT_GAUGE_SCALAR | VT_HISTOGRAM,
    };

    constexpr bool is_prometheus(VariableType vt) {
        return (static_cast<uint8_t>(vt) & static_cast<uint8_t>(VariableType::VT_PROMETHEUS)) != 0;
    }

    /**
     * @ingroup turbo_profiling_dumper
     * @brief The SnapshotFamily struct is the base class for all snapshots.
     *        Dumper implementations should use this class to access the common
     *        fields of all snapshots. drives hold the data field.
     */
    struct SnapshotFamily {
        std::string name;
        std::string description;
        std::map<std::string, std::string> labels;
        VariableType type;
        int64_t timestamp_ms{turbo::to_unix_millis(turbo::time_now())};
    };

    struct PlainStringSnapshot : public SnapshotFamily {
        std::string value;
    };

    struct CounterSnapshot : public SnapshotFamily {
        double value{0.0};
    };

    struct GaugeSnapshot : public SnapshotFamily {
        double value{0.0};
    };

    struct HistogramSnapshot : public SnapshotFamily {
        double count{0};
        double sum{0};
        double avg{0};
        std::vector<double> bins;
        std::vector<double> boundaries;
    };

    struct ObjectSnapshot : public SnapshotFamily {
        std::string value;
        std::string type_id;
    };

    /**
     * @ingroup turbo_profiling_dumper
     * @brief The VariableSnapshot struct is the base class for all snapshots.
     *        Dumper serializers should use this class to access the variable
     *        snapshot.
     *        Example:
     *        @code {.cpp}
     *        VariableSnapshot snapshot = variable->get_snapshot();
     *        const CounterSnapshot* counter = std::get_if<CounterSnapshot>(&snapshot);
     *        if(counter) {
     *          // do something
     *        }
     *        @endcode
     */
    using VariableSnapshot = std::variant<PlainStringSnapshot, CounterSnapshot, GaugeSnapshot,
            HistogramSnapshot, ObjectSnapshot>;

    using VariableSnapshotList = std::vector<VariableSnapshot>;

}  // namespace turbo

#endif  // TURBO_PROFILING_SNAPSHOT_H_
