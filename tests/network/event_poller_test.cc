
#include <csignal>
#include <iostream>
#include "turbo/network/util/util.h"
#include "turbo/network/util/logger.h"
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/poller/event_poller.h"

using namespace std;
using namespace turbo;

/**
 * cpu负载均衡测试
 * @return
 */
int main() {
    static bool  exit_flag = false;
    signal(SIGINT, [](int) { exit_flag = true; });
    //设置日志
    Logger::Instance().add(std::make_shared<ConsoleChannel>());

    Ticker ticker;
    while(!exit_flag){

        if(ticker.elapsedTime() > 1000){
            auto vec = EventPollerPool::Instance().getExecutorLoad();
            _StrPrinter printer;
            for(auto load : vec){
                printer << load << "-";
            }
            DebugL << "cpu负载:" << printer;

            EventPollerPool::Instance().getExecutorDelay([](const vector<int> &vec){
                _StrPrinter printer;
                for(auto delay : vec){
                    printer << delay << "-";
                }
                DebugL << "cpu任务执行延时:" << printer;
            });
            ticker.resetTime();
        }

        EventPollerPool::Instance().getExecutor()->async([](){
            auto usec = rand() % 4000;
            //DebugL << usec;
            usleep(usec);
        });

        usleep(2000);
    }

    return 0;
}
