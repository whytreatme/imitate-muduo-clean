#include "EchoServer.h"
#include <cstdio>
#include <unistd.h>
#include <sys/syscall.h>
#include "Logger.h"


EchoServer::EchoServer(const std::string &ip, const uint16_t port, int subnums_threads, int trnumsthreads, int timerIntervalSec, int idleTimeoutSec)
    : timerIntervalSec_(timerIntervalSec),
       idleTimeoutSec_(idleTimeoutSec),
       tcpserver_(ip, port, subnums_threads, timerIntervalSec_, idleTimeoutSec_), threadpool_(trnumsthreads, "WORKS")

{
    tcpserver_.setcloseCallback(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorCallback(std::bind(&EchoServer::HandleError, this, std::placeholders::_1));
    tcpserver_.setnewConnectionCallback(std::bind(&EchoServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setonMessageCallback(std::bind(&EchoServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendCompleteCallback(std::bind(&EchoServer::HandleSendComplete, this, std::placeholders::_1));
    //tcpserver_.settimeOutCallback(std::bind(&EchoServer::HandleTimeout, this, std::placeholders::_1));
}

EchoServer::~EchoServer()
{
  
}

void EchoServer::Start()
{
    tcpserver_.start();
}

void EchoServer::Stop()
{
    printf("[EchoServer] Stop begin.\n");
    threadpool_.stopAndjoin();
    tcpserver_.stop();
    printf("[EchoServer] Stop end.\n");
}

// 处理新客户端连接请求，在TcpServer类中回调此函数。
void EchoServer::HandleNewConnection(spConnection conn)
{
    LOG("EchoServer::HandleNewConnection - New client connected, fd=%d.", conn->fd());
    // std::cout << "New Connection Come in." << std::endl;
    //printf("EchoServer::HandleNewConnection() thread is %ld.\n",syscall(SYS_gettid));

    // 根据业务的需求，在这里可以增加其它的代码。
}


void EchoServer::HandleMessage(spConnection conn, std::string& msg)
{
   
   LOG("EchoServer::HandleMessage(fd=%d) - Received message. Adding OnMessage task to worker thread pool.", conn->fd()); 
   if(threadpool_.size() == 0){
        OnMessage(conn, msg);
   }
   else
    threadpool_.addTask("EchoServer::OnMessage", std::bind(&EchoServer::OnMessage, this, conn, msg));
}

void EchoServer::OnMessage(spConnection conn, std::string msg)
{
    if(!conn->isConnect()){
        LOG("EchoServer::OnMessage - 发现无效任务，直接丢弃，释放工作线程！");
        return; 
    }
    LOG("EchoServer::OnMessage(fd=%d) - Worker thread is now processing the message.", conn->fd());
    msg = "reply:" + msg;
   
    //printf("处理完业务之后，将使用Connection对象。\n");
    LOG("EchoServer::OnMessage(fd=%d) - Work complete. Sending reply back.", conn->fd());
    conn->send(msg.data(), msg.size());
}

void EchoServer::HandleClose(spConnection conn)
{
    LOG("EchoServer::HandleClose - Client disconnected, fd=%d.", conn->fd());
    //std::cout << "EchoServer conn closed." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}

void EchoServer::HandleError(spConnection)
{
    LOG("EchoServer::HandleError - Client error.");

    // 根据业务的需求，在这里可以增加其它的代码。
}

void EchoServer::HandleSendComplete(spConnection)
{
    LOG("EchoServer::HandleSendComplete - Message send complete.");

    // 根据业务的需求，在这里可以增加其它的代码。
}

/*void EchoServer::HandleTimeout(EventLoop*)
{
    LOG("EchoServer::HandleTimeout - timeout.");

    // 根据业务的需求，在这里可以增加其它的代码。
}*/
