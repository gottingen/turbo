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

#include "testing/sstream_workaround.h"
#include "testing/gtest_wrap.h"

#include <pthread.h>                                // pthread_*
#include <cstddef>
#include <memory>
#include <iostream>

#include "flare/times/time.h"
#include "flare/metrics/detail/agent_group.h"
#include "flare/base/static_atomic.h"


namespace {
using namespace flare::metrics_detail;

struct Add {
    uint64_t operator()(const uint64_t lhs, const uint64_t rhs) const {
        return lhs + rhs;
    }
};

const size_t OPS_PER_THREAD = 2000000;

class AgentGroupTest : public testing::Test {
protected:
    typedef std::atomic<uint64_t>                             agent_type;
    void SetUp() {}
    void TearDown() {}

    static void *thread_counter(void *arg) {
        int id = (int)((long)arg);
        agent_type *item = agent_group<agent_type>::get_or_create_tls_agent(id);
        if (item == nullptr) {
            EXPECT_TRUE(false);
            return nullptr;
        }
        flare::stop_watcher timer;
        timer.start();
        for (size_t i = 0; i < OPS_PER_THREAD; ++i) {
            agent_type *element = agent_group<agent_type>::get_or_create_tls_agent(id);
            uint64_t old_value = element->load(std::memory_order_relaxed);
            uint64_t new_value;
            do {
                new_value = old_value + 2;
            } while (__builtin_expect(!element->compare_exchange_weak(old_value, new_value, 
                                                     std::memory_order_relaxed,
                                                     std::memory_order_relaxed), 0));
            //element->store(element->load(std::memory_order_relaxed) + 2,
            //               std::memory_order_relaxed);
            //element->fetch_add(2, std::memory_order_relaxed);
        }
        timer.stop();
        return (void *)(timer.n_elapsed());
    }
};

TEST_F(AgentGroupTest, test_sanity) {
    int id = agent_group<agent_type>::create_new_agent();
    ASSERT_TRUE(id >= 0) << id;
    agent_type *element = agent_group<agent_type>::get_or_create_tls_agent(id);
    ASSERT_TRUE(element != nullptr);
    agent_group<agent_type>::destroy_agent(id);
}

std::atomic<uint64_t> g_counter(0);

void *global_add(void *) {
    flare::stop_watcher timer;
    timer.start();
    for (size_t i = 0; i < OPS_PER_THREAD; ++i) {
        g_counter.fetch_add(2, std::memory_order_relaxed);
    }
    timer.stop();
    return (void *)(timer.n_elapsed());
}

TEST_F(AgentGroupTest, test_perf) {
    size_t loops = 100000;
    size_t id_num = 512;
    int ids[id_num];
    for (size_t i = 0; i < id_num; ++i) {
        ids[i] = agent_group<agent_type>::create_new_agent();
        ASSERT_TRUE(ids[i] >= 0);
    }
    flare::stop_watcher timer;
    timer.start();
    for (size_t i = 0; i < loops; ++i) {
        for (size_t j = 0; j < id_num; ++j) {
            agent_type *agent =
                agent_group<agent_type>::get_or_create_tls_agent(ids[j]);
            ASSERT_TRUE(agent != nullptr) << ids[j];
        }
    }
    timer.stop();
    FLARE_LOG(INFO) << "It takes " << timer.n_elapsed() / (loops * id_num)
              << " ns to get tls agent for " << id_num << " agents";
    for (size_t i = 0; i < id_num; ++i) {
        agent_group<agent_type>::destroy_agent(ids[i]);
    }

}

TEST_F(AgentGroupTest, test_all_perf) {
    long id = agent_group<agent_type>::create_new_agent();
    ASSERT_TRUE(id >= 0) << id;
    pthread_t threads[24];
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
        pthread_create(&threads[i], nullptr, &thread_counter, (void *)id);
    }
    long totol_time = 0;
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
        void *ret; 
        pthread_join(threads[i], &ret);
        totol_time += (long)ret;
    }
    FLARE_LOG(INFO) << "ThreadAgent takes "
              << totol_time / (OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads));
    totol_time = 0;
    g_counter.store(0, std::memory_order_relaxed);
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
        pthread_create(&threads[i], nullptr, global_add, (void *)id);
    }
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
        void *ret; 
        pthread_join(threads[i], &ret);
        totol_time += (long)ret;
    }
    FLARE_LOG(INFO) << "Global Atomic takes "
              << totol_time / (OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads));
    agent_group<agent_type>::destroy_agent(id);
    //sleep(1000);
}
} // namespace
