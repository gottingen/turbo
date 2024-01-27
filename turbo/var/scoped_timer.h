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


#ifndef  TURBO_VAR_SCOPED_TIMER_H_
#define  TURBO_VAR_SCOPED_TIMER_H_

#include "turbo/times/time.h"

namespace turbo {
    template<typename T>
    class ScopedTimer {
    public:
        explicit ScopedTimer(T &var)
                : _start_time(turbo::Time::time_now()), _var(&var) {}

        ~ScopedTimer() {
            *_var << (turbo::Time::time_now() - _start_time);
        }

        void reset() { _start_time = turbo::Time::time_now(); }

    private:TURBO_NON_COPYABLE(ScopedTimer);
        turbo::Time _start_time;
        T *_var;
    };
} // namespace turbo

#endif  // TURBO_VAR_SCOPED_TIMER_H_