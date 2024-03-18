// Copyright 2024 The turbo Authors.
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

#ifndef TURBO_NETWORK_UTIL_LOCAL_TIME_H_
#define TURBO_NETWORK_UTIL_LOCAL_TIME_H_

#include <time.h>

namespace turbo {
void no_locks_localtime(struct tm *tmp, time_t t);
void local_time_init();
int get_daylight_active();

} // namespace turbo
#endif // TURBO_NETWORK_UTIL_LOCAL_TIME_H_