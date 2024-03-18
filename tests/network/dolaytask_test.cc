
#include <csignal>
#include <iostream>
#include "turbo/network/util/util.h"
#include "turbo/network/util/logger.h"
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/util/once_token.h"
#include "turbo/network/poller/event_poller.h"

using namespace std;
using namespace turbo;

int main() {
    //设置日志
    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

    Ticker ticker0;
    int nextDelay0 = 50;
    std::shared_ptr<onceToken> token0 = std::make_shared<onceToken>(nullptr,[](){
        TraceL << "task 0 被取消，可以立即触发释放lambad表达式捕获的变量!";
    });
    auto tag0 = EventPollerPool::Instance().getPoller()->doDelayTask(nextDelay0, [&,token0]() {
        TraceL << "task 0(固定延时重复任务),预期休眠时间 :" << nextDelay0 << " 实际休眠时间" << ticker0.elapsedTime();
        ticker0.resetTime();
        return nextDelay0;
    });
    token0 = nullptr;

    Ticker ticker1;
    int nextDelay1 = 50;
    auto tag1 = EventPollerPool::Instance().getPoller()->doDelayTask(nextDelay1, [&]() {
        DebugL << "task 1(可变延时重复任务),预期休眠时间 :" << nextDelay1 << " 实际休眠时间" << ticker1.elapsedTime();
        ticker1.resetTime();
        nextDelay1 += 1;
        return nextDelay1;
    });

    Ticker ticker2;
    int nextDelay2 = 3000;
    auto tag2 = EventPollerPool::Instance().getPoller()->doDelayTask(nextDelay2, [&]() {
        InfoL << "task 2(单次延时任务),预期休眠时间 :" << nextDelay2 << " 实际休眠时间" << ticker2.elapsedTime();
        return 0;
    });

    Ticker ticker3;
    int nextDelay3 = 50;
    auto tag3 = EventPollerPool::Instance().getPoller()->doDelayTask(nextDelay3, [&]() -> uint64_t {
        throw std::runtime_error("task 2(测试延时任务中抛异常,将导致不再继续该延时任务)");
    });


    sleep(2);
    tag0->cancel();
    tag1->cancel();
    WarnL << "取消task 0、1";

    //退出程序事件处理
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();
    return 0;
}
