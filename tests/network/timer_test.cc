
#include <csignal>
#include <iostream>
#include "turbo/network/util/util.h"
#include "turbo/network/util/logger.h"
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/poller/timer.h"

using namespace std;
using namespace turbo;

int main() {
    //设置日志
    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());


    Ticker ticker0;
    Timer::Ptr timer0 = std::make_shared<Timer>(0.5f,[&](){
        TraceL << "timer0重复:" << ticker0.elapsedTime();
        ticker0.resetTime();
        return true;
    }, nullptr);

    Timer::Ptr timer1 = std::make_shared<Timer>(1.0f,[](){
        DebugL << "timer1不再重复";
        return false;
    },nullptr);

    Ticker ticker2;
    Timer::Ptr timer2 = std::make_shared<Timer>(2.0f,[&]() -> bool {
        InfoL << "timer2,测试任务中抛异常" << ticker2.elapsedTime();
        ticker2.resetTime();
        throw std::runtime_error("timer2,测试任务中抛异常");
    },nullptr);

    //退出程序事件处理
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();
    return 0;
}