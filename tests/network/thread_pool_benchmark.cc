#include <csignal>
#include <atomic>
#include <iostream>
#include "turbo/network/util/logger.h"
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/thread/thread_pool.h"

using namespace std;
using namespace turbo;

int main() {
    signal(SIGINT,[](int ){
        exit(0);
    });
    //初始化日志系统
    Logger::Instance().add(std::make_shared<ConsoleChannel> ());

    atomic_llong count(0);
    ThreadPool pool(1, ThreadPool::Priority::PRIORITY_HIGHEST, false);

    Ticker ticker;
    for (int i = 0 ; i < 1000*10000;++i){
        pool.async([&](){
           if(++count >= 1000*10000){
               InfoL << "执行1000万任务总共耗时:" << ticker.elapsedTime() << "ms";
           }
        });
    }
    InfoL << "1000万任务入队耗时:" << ticker.elapsedTime() << "ms" << endl;
    uint64_t  lastCount = 0 ,nowCount = 1;
    ticker.resetTime();
    //此处才开始启动线程
    pool.start();
    while (true){
        sleep(1);
        nowCount = count.load();
        InfoL << "每秒执行任务数:" << nowCount - lastCount;
        if(nowCount - lastCount == 0){
            break;
        }
        lastCount = nowCount;
    }
    return 0;
}
