
#include <csignal>
#include <atomic>
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/util/logger.h"
#include "turbo/network/thread/thread_group.h"
#include "turbo/network/thread/semaphore.h"

using namespace std;
using namespace turbo;


#define MAX_TASK_SIZE (1000 * 10000)
semaphore g_sem;//信号量
atomic_llong g_produced(0);
atomic_llong g_consumed(0);

//消费者线程
void onConsum() {
    while (true) {
        g_sem.wait();
        if (++g_consumed > g_produced) {
            //如果打印这句log则表明有bug
            ErrorL << g_consumed << " > " << g_produced;
        }
    }
}

//生产者线程
void onProduce() {
    while(true){
        ++ g_produced;
        g_sem.post();
        if(g_produced >= MAX_TASK_SIZE){
            break;
        }
    }
}
int main() {
    //初始化log
    Logger::Instance().add(std::make_shared<ConsoleChannel>());

    Ticker ticker;
    thread_group thread_producer;
    for (size_t i = 0; i < thread::hardware_concurrency(); ++i) {
        thread_producer.create_thread([]() {
            //1个生产者线程
            onProduce();
        });
    }

    thread_group thread_consumer;
    for (int i = 0; i < 4; ++i) {
        thread_consumer.create_thread([i]() {
            //4个消费者线程
            onConsum();
        });
    }



    //等待所有生成者线程退出
    thread_producer.join_all();
    DebugL << "生产者线程退出，耗时:" << ticker.elapsedTime() << "ms," << "生产任务数:" << g_produced << ",消费任务数:" << g_consumed;

    int i = 5;
    while(-- i){
        DebugL << "程序退出倒计时:" << i << ",消费任务数:" << g_consumed;
        sleep(1);
    }

    //程序强制退出可能core dump；在程序推出时，生产的任务数应该跟消费任务数一致
    WarnL << "强制关闭消费线程，可能触发core dump" ;
    return 0;
}