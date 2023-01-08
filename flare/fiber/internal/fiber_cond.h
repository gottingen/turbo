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

// Date: 2015/12/14 21:26:26

#ifndef  FLARE_FIBER_INTERNAL_FIBER_COND_H_
#define  FLARE_FIBER_INTERNAL_FIBER_COND_H_

#include "flare/times/time.h"
#include "flare/fiber/internal/mutex.h"

__BEGIN_DECLS
extern int fiber_cond_init(fiber_cond_t *__restrict cond,
                           const fiber_condattr_t *__restrict cond_attr);
extern int fiber_cond_destroy(fiber_cond_t *cond);
extern int fiber_cond_signal(fiber_cond_t *cond);
extern int fiber_cond_broadcast(fiber_cond_t *cond);
extern int fiber_cond_wait(fiber_cond_t *__restrict cond,
                           fiber_mutex_t *__restrict mutex);
extern int fiber_cond_timedwait(
        fiber_cond_t *__restrict cond,
        fiber_mutex_t *__restrict mutex,
        const struct timespec *__restrict abstime);
__END_DECLS

#endif  // FLARE_FIBER_INTERNAL_FIBER_COND_H_
