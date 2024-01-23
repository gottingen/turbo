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


#include <limits>                           //std::numeric_limits
#include "turbo/var/internal/sampler.h"
#include "turbo/log/logging.h"
#include "turbo/times/time.h"
#include <gtest/gtest.h>

namespace {

    TEST(SamplerTest, linked_list) {
        turbo::intrusive_list_node n1, n2;
        n1.insert_before_as_list(&n2);
        ASSERT_EQ(n1.next, &n2);
        ASSERT_EQ(n1.prev, &n2);
        ASSERT_EQ(n2.next, &n1);
        ASSERT_EQ(n2.prev, &n1);

        turbo::intrusive_list_node n3, n4;
        n3.insert_before_as_list(&n4);
        ASSERT_EQ(n3.next, &n4);
        ASSERT_EQ(n3.prev, &n4);
        ASSERT_EQ(n4.next, &n3);
        ASSERT_EQ(n4.prev, &n3);

        n1.insert_before_as_list(&n3);
        ASSERT_EQ(n1.next, &n2);
        ASSERT_EQ(n2.next, &n3);
        ASSERT_EQ(n3.next, &n4);
        ASSERT_EQ(n4.next, &n1);
        ASSERT_EQ(&n1, n2.prev);
        ASSERT_EQ(&n2, n3.prev);
        ASSERT_EQ(&n3, n4.prev);
        ASSERT_EQ(&n4, n1.prev);
    }

    class DebugSampler : public turbo::var_internal::Sampler {
    public:
        DebugSampler() : _ncalled(0) {}

        ~DebugSampler() {
            ++_s_ndestroy;
        }

        void take_sample() {
            ++_ncalled;
        }

        int called_count() const { return _ncalled; }

    private:
        int _ncalled;
        static int _s_ndestroy;
    };

    int DebugSampler::_s_ndestroy = 0;

    TEST(SamplerTest, single_threaded) {
        const int N = 100;
        DebugSampler *s[N];
        for (int i = 0; i < N; ++i) {
            s[i] = new DebugSampler;
            s[i]->schedule();
        }
        usleep(1010000);
        for (int i = 0; i < N; ++i) {
            // LE: called once every second, may be called more than once
            ASSERT_LE(1, s[i]->called_count()) << "i=" << i;
        }
        EXPECT_EQ(0, DebugSampler::_s_ndestroy);
        for (int i = 0; i < N; ++i) {
            s[i]->destroy();
        }
        usleep(1010000);
        EXPECT_EQ(N, DebugSampler::_s_ndestroy);
    }

    static void *check(void *) {
        const int N = 100;
        DebugSampler *s[N];
        for (int i = 0; i < N; ++i) {
            s[i] = new DebugSampler;
            s[i]->schedule();
        }
        usleep(1010000);
        for (int i = 0; i < N; ++i) {
            EXPECT_LE(1, s[i]->called_count()) << "i=" << i;
        }
        for (int i = 0; i < N; ++i) {
            s[i]->destroy();
        }
        return NULL;
    }

    TEST(SamplerTest, multi_threaded) {
        pthread_t th[10];
        DebugSampler::_s_ndestroy = 0;
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_create(&th[i], NULL, check, NULL));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], NULL));
        }
        sleep(1);
        EXPECT_EQ(100 * TURBO_ARRAY_SIZE(th), (size_t) DebugSampler::_s_ndestroy);
    }
} // namespace
