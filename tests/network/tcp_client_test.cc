#include <csignal>
#include <iostream>
#include "turbo/network/util/logger.h"
#include "turbo/network/network/tcp_client.h"

using namespace std;
using namespace turbo;

class TestClient: public TcpClient {
public:
    using Ptr = std::shared_ptr<TestClient>;
    TestClient():TcpClient() {
        DebugL;
    }
    ~TestClient(){
        DebugL;
    }
protected:
    virtual void onConnect(const SockException &ex) override{
        //连接结果事件
        InfoL << (ex ?  ex.what() : "success");
    }
    virtual void onRecv(const Buffer::Ptr &pBuf) override{
        //接收数据事件
        DebugL << pBuf->data() << " from port:" << get_peer_port();
    }
    virtual void onFlush() override{
        //发送阻塞后，缓存清空事件
        DebugL;
    }
    virtual void onError(const SockException &ex) override{
        //断开连接事件，一般是EOF
        WarnL << ex.what();
    }
    virtual void onManager() override{
        //定时发送数据到服务器
        auto buf = BufferRaw::create();
        if(buf){
            buf->assign("[BufferRaw]\0");
            (*this) << _nTick++ << " "
                    << 3.14 << " "
                    << string("string") << " "
                    <<(Buffer::Ptr &)buf;
        }
    }
private:
    int _nTick = 0;
};


int main() {
    // 设置日志系统
    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

    TestClient::Ptr client(new TestClient());//必须使用智能指针
    client->startConnect("127.0.0.1",9000);//连接服务器

    TcpClientWithSSL<TestClient>::Ptr clientSSL(new TcpClientWithSSL<TestClient>());//必须使用智能指针
    clientSSL->startConnect("127.0.0.1",9001);//连接服务器

    //退出程序事件处理
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();
    return 0;
}