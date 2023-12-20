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

#ifndef TURBO_PROFILING_DUMPER_H_
#define TURBO_PROFILING_DUMPER_H_

#include "turbo/profiling/snapshot.h"
#include <ostream>
namespace turbo {

    enum class DumperType : uint8_t {
        DUMP_PLAIN_NONE = 0,
        DUMP_PLAIN_TEXT = 1<<0,
        DUMP_PLAIN_JSON = 1<<1,
        DUMP_PLAIN_HTML = 1<<2,
        DUMP_PLAIN_PROMETHEUS = 1<<3,
    };

    constexpr DumperType operator|(DumperType a, DumperType b) {
        return static_cast<DumperType>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    constexpr DumperType operator&(DumperType a, DumperType b) {
        return static_cast<DumperType>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    constexpr DumperType operator^(DumperType a, DumperType b) {
        return static_cast<DumperType>(static_cast<uint8_t>(a) ^ static_cast<uint8_t>(b));
    }

    constexpr DumperType operator~(DumperType a) {
        return static_cast<DumperType>(~static_cast<uint8_t>(a));
    }

    constexpr DumperType &operator|=(DumperType &a, DumperType b) {
        return a = a | b;
    }

    constexpr DumperType &operator&=(DumperType &a, DumperType b) {
        return a = a & b;
    }

    constexpr DumperType &operator^=(DumperType &a, DumperType b) {
        return a = a ^ b;
    }

    constexpr bool operator!(DumperType a) {
        return !static_cast<uint8_t>(a);
    }

    constexpr bool is_supported_text(DumperType a) {
        return (a & DumperType::DUMP_PLAIN_TEXT) == DumperType::DUMP_PLAIN_TEXT;
    }

    constexpr bool is_supported_json(DumperType a) {
        return (a & DumperType::DUMP_PLAIN_JSON) == DumperType::DUMP_PLAIN_JSON;
    }

    constexpr bool is_supported_html(DumperType a) {
        return (a & DumperType::DUMP_PLAIN_HTML) == DumperType::DUMP_PLAIN_HTML;
    }

    constexpr bool is_supported_prometheus(DumperType a) {
        return (a & DumperType::DUMP_PLAIN_PROMETHEUS) == DumperType::DUMP_PLAIN_PROMETHEUS;
    }

    constexpr bool is_supported_none(DumperType a) {
        return a == DumperType::DUMP_PLAIN_NONE;
    }

    static constexpr DumperType DUMP_ALL = DumperType::DUMP_PLAIN_TEXT | DumperType::DUMP_PLAIN_HTML | DumperType::DUMP_PLAIN_PROMETHEUS | DumperType::DUMP_PLAIN_JSON;

    static constexpr DumperType DUMP_PLAIN_TEXT = DumperType::DUMP_PLAIN_TEXT | DumperType::DUMP_PLAIN_HTML | DumperType::DUMP_PLAIN_JSON;

    static constexpr DumperType DUMP_PROMETHEUS_TYPE = DumperType::DUMP_PLAIN_PROMETHEUS | DumperType::DUMP_PLAIN_TEXT| DumperType::DUMP_PLAIN_HTML;


    struct VariableDumper {
        virtual ~VariableDumper() = default;

        virtual void dump(std::ostream &os, const VariableSnapshot &snapshot) {
            os<<dump(snapshot);
        }

        virtual std::string dump(const VariableSnapshot &snapshot) = 0;
    };

}  // namespace turbo

#endif  // TURBO_PROFILING_DUMPER_H_
