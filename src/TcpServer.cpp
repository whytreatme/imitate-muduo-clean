#include "TcpServer.h"
#include "Channel.h"
#include <cstdio>
#include <functional>
#include "Logger.h"
#include <vector>




TcpServer::TcpServer(const std::string &ip, const uint16_t port, int numsthreads, int timerIntervalSec, int idleTimeoutSec) 
            : timerIntervalSec_(timerIntervalSec),
              idleTimeoutSec_(idleTimeoutSec),
              mainloop_(), nums_threads(numsthreads), threadpool_(nums_threads, "IO"),
              acceptor_(mainloop_, ip, port)
              
{
  // mainloop_.setepollTimeoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));  
   acceptor_.setnewConnection(std::bind(&TcpServer::newConnection, this, std::placeholders::_1));

   //创建从事件循环
   for(int i = 0; i < nums_threads; i++)
   {
        subloops_.emplace_back(new EventLoop(timerIntervalSec_));
        subloops_[i]->setTimeoutCallback(std::bind(&TcpServer::epollTimeout, this ,std::placeholders::_1));
       // subloops_[i]->setepollTimeoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));  
        threadpool_.addTask(" EventLoop", std::bind(&EventLoop::run, subloops_[i].get()));
   }
}

TcpServer::~TcpServer()
{
    //delete acceptor_;
    //delete mainloop_;

    // for(auto &aa : conns_)
    // {
    //     delete aa.second;
    // }  

    // for(auto &el : subloops_)
    // {
    //     delete el;
    // }
}

void TcpServer::start()
{
    printf("[TcpServer] start: mainloop run begin.\n");
    {
        std::lock_guard<std::mutex> lock(stopMutex_);
        mainloopStopped_ = false;
    }
    mainloopRunning_.store(true, std::memory_order_release);
    mainloop_.run();
    {
        std::lock_guard<std::mutex> lock(stopMutex_);
        mainloopStopped_ = true;
    }
    mainloopRunning_.store(false, std::memory_order_release);
    stopCv_.notify_all();
    printf("[TcpServer] start: mainloop run end.\n");
}

void TcpServer::stop()
{
    printf("[TcpServer] stop begin.\n");
    mainloop_.queueInLoop(std::bind(&Acceptor::closeListenNewconnection, &acceptor_));
    std::vector<spConnection> connsBakcup;
    {
        std::lock_guard<std::mutex> lock(mapMutex_);
        for(auto &[fd, conn] : conns_){
            connsBakcup.push_back(conn);
        }
    }
    printf("[TcpServer] stop: closing %zu connections.\n", connsBakcup.size());
    for(auto & conn : connsBakcup){
        conn->close();
    }
    
    for(int i = 0; i <nums_threads; i++)
        subloops_[i]->queueInLoop(std::bind(&EventLoop::stop, subloops_[i].get()));
    printf("[TcpServer] stop: waiting for subloops...\n");
    threadpool_.stopAndjoin();
    printf("[TcpServer] stop: subloops exited.\n");
    mainloop_.queueInLoop(std::bind(&EventLoop::stop, &mainloop_));
    if(mainloopRunning_.load(std::memory_order_acquire)){
        printf("[TcpServer] stop: waiting for mainloop...\n");
        std::unique_lock<std::mutex> lock(stopMutex_);
        stopCv_.wait(lock, [this]{ return mainloopStopped_; });
    }
    printf("[TcpServer] stop end.\n");
}

void TcpServer::newConnection(std::unique_ptr<Socket> clientsock)
{
    LOG("TcpServer::newConnection - Handing new connection for fd=%d", clientsock->fd());
    //printf("接收到新的连接。\n");
    //Connection* conn = new Connection(mainloop_, clientsock);
    //把Connection分配给从事件循环
    EventLoop *belongLoop = subloops_[clientsock->fd() % nums_threads].get();
    spConnection conn = std::make_shared<Connection>(*belongLoop, std::move(clientsock), idleTimeoutSec_);
    LOG("TcpServer::newConnection - Setting callbacks for fd=%d", conn->fd());
    conn->setCloseCallback(std::bind(&TcpServer::closeConnection, this, std::placeholders::_1));
    conn->setErrorCallback(std::bind(&TcpServer::errorConnection, this, std::placeholders::_1));
    conn->setonMessageCallback(std::bind(&TcpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    conn->setsendCompleteCallback(std::bind(&TcpServer::sendComplete, this, std::placeholders::_1));
    //printf ("new Connection(fd=%d,ip=%s,port=%d) ok.\n",conn->fd(),conn->ip().c_str(),conn->port());
    LOG("TcpServer::newConnection - Callbacks set for fd=%d", conn->fd());
    {
    std::lock_guard<std::mutex> lock(mapMutex_);
    conns_[conn->fd()] = conn; 
    loopConns_[belongLoop].emplace(conn->fd(), conn);
    }
    if(newConnectionCallback_) {
        LOG("TcpServer::newConnection - Invoking application-level new connection callback for fd=%d", conn->fd());
        newConnectionCallback_(conn);
    }

    conn->ConnectEstablished();
}

void TcpServer::errorConnection(spConnection conn)
{
    LOG("TcpServer::errorConnection - Connection error for fd=%d", conn->fd());
    if(errorCallback_) errorCallback_(conn);
    std::lock_guard<std::mutex> lock(mapMutex_);
    conns_.erase(conn->fd());
    auto it  = loopConns_.find(conn->getloop());
    if(it == loopConns_.end()) return;
    it->second.erase(conn->fd());
   
}

//将接受到的报文计算并发回
void TcpServer::onMessage(spConnection conn, std::string& msg)
{
    LOG("TcpServer::onMessage - Received message from fd=%d, passing to application callback.", conn->fd());
    //printf("我现在在TcpServer::onMessage里！\n");
    if(onMessageCallback_) onMessageCallback_(conn, msg);
  
}

void TcpServer::closeConnection(spConnection conn)
{
    LOG("TcpServer::closeConnection - Closing connection for fd=%d", conn->fd());
    if(closeCallback_) closeCallback_(conn);
    std::lock_guard<std::mutex> lock(mapMutex_);
    conns_.erase(conn->fd());
    auto it  = loopConns_.find(conn->getloop());
    if(it == loopConns_.end()) return;
    it->second.erase(conn->fd());
}



void TcpServer::sendComplete(spConnection conn)
{   
    if(sendComleteCallback_) sendComleteCallback_(conn);
    //printf("send complete.\n");
    
    //根据业务需求增加代码
}

void TcpServer::epollTimeout(EventLoop *loop)
{
    std::vector<spConnection> expirationConns;
    {
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto it = loopConns_.find(loop);
    if(it == loopConns_.end()) return;

    auto &connMap = it->second;
    
    for(const auto & [fd, conn] : connMap){
        if(conn->isIdle())
            expirationConns.push_back(conn);
    }
    }
    for(auto & conn : expirationConns){
        if(!conn->isConnect()) continue;
        conn->close();
        //closeConnection(conn);
    }
   
}

void TcpServer::setnewConnectionCallback(std::function<void (spConnection)> fn)
{
    newConnectionCallback_ = fn;
}

void TcpServer::setonMessageCallback(std::function<void (spConnection, std::string&)> fn)
{
    onMessageCallback_ = fn;
}

void TcpServer::setcloseCallback(std::function<void (spConnection)> fn)
{
    closeCallback_ = fn;
}   

void TcpServer::seterrorCallback(std::function<void (spConnection)> fn)
{
    errorCallback_ = fn;
}  

void TcpServer::setsendCompleteCallback(std::function<void (spConnection)> fn)
{
    sendComleteCallback_ = fn;
}  

void TcpServer::settimeOutCallback(std::function<void(EventLoop*)> fn)
{
    timeOutCallback_ = fn;
}

