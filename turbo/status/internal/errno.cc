// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: Wed Jul 30 11:47:19 CST 2014

#include "turbo/status/internal/errno.h"
#include "turbo/platform/port.h"

extern "C" {

#if defined(TURBO_PLATFORM_LINUX)

extern int *__errno_location() __attribute__((__const__));

int *fiber_errno_location() {
    return __errno_location();
}
#elif defined(TURBO_PLATFORM_OSX)

extern int * __error(void);

int *fiber_errno_location() {
    return __error();
}
#endif

}  // extern "C"
