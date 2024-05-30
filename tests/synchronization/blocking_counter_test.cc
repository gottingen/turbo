// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <turbo/synchronization/blocking_counter.h>

#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include <turbo/times/clock.h>
#include <turbo/times/time.h>

namespace turbo {
    TURBO_NAMESPACE_BEGIN
    namespace {

        void PauseAndDecreaseCounter(BlockingCounter *counter, int *done) {
            turbo::sleep_for(turbo::Duration::seconds(1));
            *done = 1;
            counter->DecrementCount();
        }

        TEST(BlockingCounterTest, BasicFunctionality) {
            // This test verifies that BlockingCounter functions correctly. Starts a
            // number of threads that just sleep for a second and decrement a counter.

            // Initialize the counter.
            const int num_workers = 10;
            BlockingCounter counter(num_workers);

            std::vector<std::thread> workers;
            std::vector<int> done(num_workers, 0);

            // Start a number of parallel tasks that will just wait for a seconds and
            // then decrement the count.
            workers.reserve(num_workers);
            for (int k = 0; k < num_workers; k++) {
                workers.emplace_back(
                        [&counter, &done, k] { PauseAndDecreaseCounter(&counter, &done[k]); });
            }

            // Wait for the threads to have all finished.
            counter.Wait();

            // Check that all the workers have completed.
            for (int k = 0; k < num_workers; k++) {
                EXPECT_EQ(1, done[k]);
            }

            for (std::thread &w: workers) {
                w.join();
            }
        }

        TEST(BlockingCounterTest, WaitZeroInitialCount) {
            BlockingCounter counter(0);
            counter.Wait();
        }

#if GTEST_HAS_DEATH_TEST
        TEST(BlockingCounterTest, WaitNegativeInitialCount) {
            EXPECT_DEATH(BlockingCounter counter(-1),
                         "BlockingCounter initial_count negative");
        }

#endif

    }  // namespace
    TURBO_NAMESPACE_END
}  // namespace turbo
