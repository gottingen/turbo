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

#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "testing/gtest_wrap.h"
#include "flare/times/time.h"
#include "flare/base/profile.h"

namespace {
void* read_thread(void* arg) {
    const size_t N = 10000;
#ifdef CHECK_RWLOCK
    pthread_rwlock_t* lock = (pthread_rwlock_t*)arg;
#else
    pthread_mutex_t* lock = (pthread_mutex_t*)arg;
#endif
    const long t1 = flare::get_current_time_nanos();
    for (size_t i = 0; i < N; ++i) {
#ifdef CHECK_RWLOCK
        pthread_rwlock_rdlock(lock);
        pthread_rwlock_unlock(lock);
#else
        pthread_mutex_lock(lock);
        pthread_mutex_unlock(lock);
#endif
    }
    const long t2 = flare::get_current_time_nanos();
    return new long((t2 - t1)/N);
}

void* write_thread(void*) {
    return nullptr;
}

TEST(RWLockTest, rdlock_performance) {
#ifdef CHECK_RWLOCK
    pthread_rwlock_t lock1;
    ASSERT_EQ(0, pthread_rwlock_init(&lock1, nullptr));
#else
    pthread_mutex_t lock1;
    ASSERT_EQ(0, pthread_mutex_init(&lock1, nullptr));
#endif
    pthread_t rth[16];
    pthread_t wth;
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(rth); ++i) {
        ASSERT_EQ(0, pthread_create(&rth[i], nullptr, read_thread, &lock1));
    }
    ASSERT_EQ(0, pthread_create(&wth, nullptr, write_thread, &lock1));
    
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(rth); ++i) {
        long* res = nullptr;
        pthread_join(rth[i], (void**)&res);
        printf("read thread %lu = %ldns\n", i, *res);
    }
    pthread_join(wth, nullptr);
#ifdef CHECK_RWLOCK
    pthread_rwlock_destroy(&lock1);
#else
    pthread_mutex_destroy(&lock1);
#endif
}
} // namespace
