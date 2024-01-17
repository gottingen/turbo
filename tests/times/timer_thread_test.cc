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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"
#include "turbo/log/logging.h"
#include "turbo/times/timer_thread.h"
#include "turbo/concurrent/spinlock_wait.h"
#include "turbo/times/stop_watcher.h"

class TimeKeeper {
public:
    TimeKeeper(turbo::Time run_time)
            : _expect_run_time(run_time), _name(nullptr), _sleep_ms(0) {}

    TimeKeeper(turbo::Time run_time, const char *name/*must be string constant*/)
            : _expect_run_time(run_time), _name(name), _sleep_ms(0) {}

    TimeKeeper(turbo::Time run_time, const char *name/*must be string constant*/,
               int sleep_ms)
            : _expect_run_time(run_time), _name(name), _sleep_ms(sleep_ms) {}

    void schedule(turbo::TimerThread *timer_thread) {
        _task_id = timer_thread->schedule(
                TimeKeeper::routine, this, _expect_run_time);
    }

    void run() {
        auto current_time = turbo::time_now();
        if (_name) {
            TLOG_INFO("Run `{0}' task_id={1}", _name, _task_id);
        } else {
            TLOG_INFO("Run task_id={0}", _task_id);
        }
        _run_times.push_back(current_time);
        const int saved_sleep_ms = _sleep_ms;
        if (saved_sleep_ms > 0) {
            _sleep_ms.wait_for(saved_sleep_ms, turbo::Duration::milliseconds(saved_sleep_ms));
        }
    }

    void wakeup() {
        if (_sleep_ms != 0) {
            _sleep_ms = 0;
            _sleep_ms.wake_one();
        } else {
            TLOG_ERROR("No need to wakeup `{0}' task_id={1}",
                       _name ? _name : "", _task_id);
        }
    }

    // verify the first run is in specified time range.
    void expect_first_run() {
        expect_first_run(_expect_run_time);
    }

    void expect_first_run(turbo::Time expect_run_time) {
        REQUIRE(!_run_times.empty());
        REQUIRE_LE((_run_times[0] - expect_run_time), turbo::Duration::microseconds(50000));
    }

    void expect_not_run() {
        REQUIRE(_run_times.empty());
    }

    static void routine(void *arg) {
        TimeKeeper *keeper = (TimeKeeper *) arg;
        keeper->run();
    }

    turbo::Time _expect_run_time;
    turbo::TimerThread::TaskId _task_id;

private:
    const char *_name;
    turbo::SpinWaiter _sleep_ms;
    std::vector<turbo::Time> _run_times;
};

TEST_CASE("TimerThreadTest, RunTasks") {
    turbo::TimerThread timer_thread;
    REQUIRE_EQ(timer_thread.start(nullptr).ok(), true);

    auto _2s_later = turbo::seconds_from_now(2);
    TimeKeeper keeper1(_2s_later, "keeper1");
    keeper1.schedule(&timer_thread);

    TimeKeeper keeper2(_2s_later, "keeper2");  // same time with keeper1
    keeper2.schedule(&timer_thread);

    auto _1s_later = turbo::seconds_from_now(1);
    TimeKeeper keeper3(_1s_later, "keeper3");
    keeper3.schedule(&timer_thread);

    auto _10s_later = turbo::seconds_from_now(10);
    TimeKeeper keeper4(_10s_later, "keeper4");
    keeper4.schedule(&timer_thread);

    TimeKeeper keeper5(_10s_later, "keeper5");
    keeper5.schedule(&timer_thread);

    // sleep 1 second, and unschedule task2
    TLOG_INFO("Sleep 1s");
    sleep(1);
    timer_thread.unschedule(keeper2._task_id);
    timer_thread.unschedule(keeper4._task_id);

    turbo::Time old_time;
    TimeKeeper keeper6(old_time, "keeper6");
    keeper6.schedule(&timer_thread);
    const turbo::Time keeper6_addtime = turbo::seconds_from_now(0);

    // sleep 10 seconds and stop.
    TLOG_INFO("Sleep 2s");
    sleep(2);
    TLOG_INFO("Stop timer_thread");
    turbo::StopWatcher tm;
    tm.reset();
    timer_thread.stop_and_join();
    tm.stop();
    REQUIRE_LE(tm.elapsed_mill(), 15);

    // verify all runs in expected time range.
    keeper1.expect_first_run();
    keeper2.expect_not_run();
    keeper3.expect_first_run();
    keeper4.expect_not_run();
    keeper5.expect_not_run();
    keeper6.expect_first_run(keeper6_addtime);
}

// If the scheduled time is before start time, then should run it
// immediately.
TEST_CASE("TimerThreadTest, start_after_schedule") {
    turbo::TimerThread timer_thread;
    turbo::Time past_time;
    TimeKeeper keeper(past_time, "keeper1");
    keeper.schedule(&timer_thread);
    REQUIRE_EQ(turbo::TimerThread::INVALID_TASK_ID, keeper._task_id);
    REQUIRE_EQ(timer_thread.start(nullptr).ok(), true);
    keeper.schedule(&timer_thread);
    REQUIRE_NE(turbo::TimerThread::INVALID_TASK_ID, keeper._task_id);
    auto current_time = turbo::seconds_from_now(0);
    sleep(1);  // make sure timer thread start and run
    timer_thread.stop_and_join();
    keeper.expect_first_run(current_time);
}

class TestTask {
public:
    TestTask(turbo::TimerThread *timer_thread, TimeKeeper *keeper1,
             TimeKeeper *keeper2, int expected_unschedule_result)
            : _timer_thread(timer_thread), _keeper1(keeper1), _keeper2(keeper2),
              _expected_unschedule_result(expected_unschedule_result) {
    }

    void run() {
        _running_time = turbo::time_now();
        REQUIRE_EQ(_expected_unschedule_result,
                   _timer_thread->unschedule(_keeper1->_task_id).map_code());
        _keeper2->schedule(_timer_thread);
    }

    static void routine(void *arg) {
        TestTask *task = (TestTask *) arg;
        task->run();
    }

    turbo::Time _running_time;

private:
    turbo::TimerThread *_timer_thread;  // not owned.
    TimeKeeper *_keeper1;  // not owned.
    TimeKeeper *_keeper2;  // not owned.
    int _expected_unschedule_result;
};

// Perform schedule and unschedule inside a running task
TEST_CASE("TimerThreadTest, schedule_and_unschedule_in_task") {
    turbo::TimerThread timer_thread;
    turbo::Time past_time;
    auto future_time = turbo::Time::infinite_future();
    const auto _500ms_after = turbo::milliseconds_from_now(500);

    TimeKeeper keeper1(future_time, "keeper1");
    TimeKeeper keeper2(past_time, "keeper2");
    TimeKeeper keeper3(past_time, "keeper3");
    TimeKeeper keeper4(past_time, "keeper4");
    TimeKeeper keeper5(_500ms_after, "keeper5", 10000/*10s*/);

    REQUIRE_EQ(timer_thread.start(nullptr).ok(), true);
    keeper1.schedule(&timer_thread);  // start keeper1
    keeper3.schedule(&timer_thread);  // start keeper3
    auto keeper3_addtime = turbo::seconds_from_now(0);
    keeper5.schedule(&timer_thread);  // start keeper5
    sleep(1);  // let keeper1/3/5 run

    TestTask test_task1(&timer_thread, &keeper1, &keeper2, turbo::kOk);
    timer_thread.schedule(TestTask::routine, &test_task1, past_time);

    TestTask test_task2(&timer_thread, &keeper3, &keeper4, turbo::kNotFound);
    timer_thread.schedule(TestTask::routine, &test_task2, past_time);

    sleep(1);
    // test_task1/2 should be both blocked by keeper5.
    keeper2.expect_not_run();
    keeper4.expect_not_run();

    // unscheduling (running) keeper5 should have no effect and returns 1
    REQUIRE(turbo::is_resource_busy(timer_thread.unschedule(keeper5._task_id)));

    // wake up keeper5 to let test_task1/2 run.
    keeper5.wakeup();
    sleep(1);

    timer_thread.stop_and_join();

    keeper1.expect_not_run();
    keeper2.expect_first_run(test_task1._running_time);
    keeper3.expect_first_run(keeper3_addtime);
    keeper4.expect_first_run(test_task2._running_time);
    keeper5.expect_first_run();
}