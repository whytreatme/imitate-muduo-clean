#pragma once
#include "TcpServer.h"
#include "EventLoop.h"
#include "Connection.h"
#include "ThreadPool.h"

class EchoServer{
private:
    int timerIntervalSec_;         //转发给TcpServer
    int idleTimeoutSec_;           //转发给TcpServer
    
    TcpServer tcpserver_;
    thpool threadpool_;
    int trnums_threads_;

   
public:
    EchoServer(const std::string &ip, const uint16_t port, int subnums_threads, int trnumsthreads=5, int timerIntervalSec=60, int idleTimeoutSec=180);
    ~EchoServer();

    void Start();    //启动服务
    void Stop();     //停止服务

    //回调作为抓手操作底层类
    void HandleNewConnection(spConnection);
    void HandleMessage(spConnection, std::string&);
    void HandleClose(spConnection);
    void HandleError(spConnection);
    void HandleSendComplete(spConnection);
    //void HandleTimeout(EventLoop*);

    void OnMessage(spConnection, std::string);           //使用工作线程执行计算任务
};