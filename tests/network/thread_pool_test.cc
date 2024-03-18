#include <chrono>
#include "turbo/network/util/logger.h"
#include "turbo/network/util/once_token.h"
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/thread/thread_pool.h"

using namespace std;
using namespace turbo;

int main() {
    //初始化日志系统
    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

    ThreadPool pool(thread::hardware_concurrency(), ThreadPool::Priority::PRIORITY_HIGHEST, true);

    //每个任务耗时3秒
    auto task_second = 3;
    //每个线程平均执行4次任务，总耗时应该为12秒
    auto task_count = thread::hardware_concurrency() * 4;

    semaphore sem;
    vector<int> vec;
    vec.resize(task_count);
    Ticker ticker;
    {
        //放在作用域中确保token引用次数减1
        auto token = std::make_shared<onceToken>(nullptr, [&]() {
            sem.post();
        });

        for (auto i = 0; i < task_count; ++i) {
            pool.async([token, i, task_second, &vec]() {
                setThreadName(("thread pool " + to_string(i)).data());
                std::this_thread::sleep_for(std::chrono::seconds(task_second)); //休眠三秒
                InfoL << "task " << i << " done!";
                vec[i] = i;
            });
        }
    }

    sem.wait();
    InfoL << "all task done, used milliseconds:" << ticker.elapsedTime();

    //打印执行结果
    for (auto i = 0; i < task_count; ++i) {
        InfoL << vec[i];
    }
    return 0;
}