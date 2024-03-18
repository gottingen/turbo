
#include <csignal>
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "turbo/network/util/logger.h"
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/network/tcp_server.h"
#include "turbo/network/network/session.h"

using namespace std;
using namespace turbo;

class EchoSession: public Session {
public:
    EchoSession(const Socket::Ptr &sock) :
            Session(sock) {
        DebugL;
    }
    ~EchoSession() {
        DebugL;
    }
    virtual void onRecv(const Buffer::Ptr &buf) override{
        //处理客户端发送过来的数据
        TraceL << buf->data() <<  " from port:" << get_local_port();
        send(buf);
    }
    virtual void onError(const SockException &err) override{
        //客户端断开连接或其他原因导致该对象脱离TCPServer管理
        WarnL << err;
    }
    virtual void onManager() override{
        //定时管理该对象，譬如会话超时检查
        DebugL;
    }

private:
    Ticker _ticker;
};


int main() {
    //初始化日志模块
    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

    //加载证书，证书包含公钥和私钥
    SSL_Initor::Instance().loadCertificate((exeDir() + "ssl.p12").data());
    SSL_Initor::Instance().trustCertificate((exeDir() + "ssl.p12").data());
    SSL_Initor::Instance().ignoreInvalidCertificate(false);

    TcpServer::Ptr server(new TcpServer());
    server->start<EchoSession>(9000);//监听9000端口

    TcpServer::Ptr serverSSL(new TcpServer());
    serverSSL->start<SessionWithSSL<EchoSession> >(9001);//监听9001端口

    //退出程序事件处理
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();
    return 0;
}