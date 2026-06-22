#pragma once
#include "EventLoop.h"
#include "Socket.h"
#include "InetAddress.h"
#include <string>
#include "Acceptor.h"
#include "Connection.h"
#include <map>
#include <unordered_map>
#include "ThreadPool.h"
#include <mutex>
#include <condition_variable>
#include <atomic>


/*
TcpServer有什么权限
TcpServer拥有Acceptor类和管理Connection类的权限
TcpServer拥有Acceptor创建和析构的权限
TcpServer“逻辑上”拥有Connection创建和析构的权限，实际上Connection由EventLoop和Channel协助完成
TcpServer负责管理所有的Connection对象
TcpServer负责处理新连接的回调
TcpServer负责处理连接的关闭和错误
TcpServer负责启动事件循环
*/
class TcpServer{
private:
    int timerIntervalSec_;         //转发给EventLoop
    int idleTimeoutSec_;           //转发给Connection

    EventLoop mainloop_;               //主线程运行主事件循环
    std::vector<std::unique_ptr<EventLoop>> subloops_;   //线程池运行从事件循环
    int nums_threads;                   //线程池的线程数
    thpool threadpool_;                //建立线程池
    
    Acceptor acceptor_;

    

    std::map<int, spConnection>conns_;                               //用于普通连接断开的共享表
    std::unordered_map<EventLoop*, std::unordered_map<int, spConnection>> loopConns_;  //用于空闲连接断开的共享表
    std::mutex mapMutex_;       //用于保护两张表的锁

    std::mutex stopMutex_;
    std::condition_variable stopCv_;
    std::atomic<bool> mainloopRunning_{false};
    bool mainloopStopped_ = false;
    //给echoserver使用的回调函数接口
    std::function<void (spConnection)> newConnectionCallback_;
    std::function<void (spConnection, std::string&)> onMessageCallback_;
    std::function<void (spConnection)> closeCallback_;
    std::function<void (spConnection)> errorCallback_;
    std::function<void (spConnection)> sendComleteCallback_;
    std::function<void (EventLoop*)> timeOutCallback_;
    
public:
    TcpServer(const std::string &ip, const uint16_t port, int numsthreads = 3, int timerIntervalSec=60, int idleTimeoutSec=180);
    ~TcpServer();

    void start();
    void stop();

    void newConnection(std::unique_ptr<Socket>);
    void onMessage(spConnection , std::string&);
    void closeConnection(spConnection);
    void errorConnection(spConnection);
    void sendComplete(spConnection);
    void epollTimeout(EventLoop*);

    void setnewConnectionCallback(std::function<void (spConnection)> fn);
    void setonMessageCallback(std::function<void (spConnection, std::string&)> fn);
    void setcloseCallback(std::function<void (spConnection)> fn);
    void seterrorCallback(std::function<void (spConnection)> fn);
    void setsendCompleteCallback(std::function<void (spConnection)> fn);
    void settimeOutCallback(std::function<void (EventLoop*)> fn);

    
};