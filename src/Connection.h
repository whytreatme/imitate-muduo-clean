#pragma once
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include <iostream>
#include <memory>
#include <atomic>
#include <mutex>
#include "Buffer.h"
#include <chrono>


class Connection;
using spConnection = std::shared_ptr<Connection>;

class Connection : public std::enable_shared_from_this<Connection>{
private:
    int idleTimeoutSec_;            //用于设置空闲时间

    EventLoop &loop_;
    std::unique_ptr<Socket> clientsock_;
    std::unique_ptr<Channel> clientChannel_;
    Buffer inputbuffer_;             //增加接受缓冲区
    Buffer outputbuffer_;            //发送接受缓冲区

    std::atomic_bool isDisconnect;   //同步机制维护线程安全的发送方法
    std::mutex mutex_;               //用于保护lastActive_

    std::chrono::steady_clock::time_point lastActive_;

    //回调函数区
    std::function<void(spConnection)> closeCallback_;
    std::function<void(spConnection)> errorCallback_;
    std::function<void(spConnection, std::string&)> onMessageCallback_;
    std::function<void(spConnection)> sendCompleteCallback_;
    
public:
    Connection(EventLoop &loop, std::unique_ptr<Socket>, int idleTimeoutSec=180);
    ~Connection();

    int fd() const;
    std::string ip() const;
    uint16_t port() const;
    void errorCallback();
    void closeCallback();
    void onMessage();
    void send(const char *data, size_t size);
    void sendInLoop(std::shared_ptr<std::string>);
    void writeCallback();

    void setErrorCallback(std::function<void(spConnection)> fn);
    void setCloseCallback(std::function<void(spConnection)> fn);
    void setonMessageCallback(std::function<void(spConnection, std::string&)> fn);
    void setsendCompleteCallback(std::function<void(spConnection)> fn);

    void ConnectEstablished();
    bool isConnect () const;

    bool isIdle();

    void close();     //用于TcpServer主动通知连接自己回收资源
    void closeInLoop();   //在对应的事件循环中关闭连接

    EventLoop* getloop() const;
};