#pragma once
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include <string>
#include <memory>

/*
Acceptor类的职责
Acceptor类负责创建监听套接字Socket对象
Acceptor类负责创建监听套接字对应的Channel对象
Acceptor类负责监听套接字的读事件回调
Acceptor类负责将新连接通过回调函数传递给上层调用者
*/
class Acceptor{
private:
    EventLoop &loop_;
    Socket servsock_;
    Channel acceptChannel_;
    std::function<void(std::unique_ptr<Socket>)> newConnectioncb_;
public:
    Acceptor(EventLoop &loop, const std::string &ip, uint16_t port);
    ~Acceptor();

    void newConnection();
    void setnewConnection( std::function<void(std::unique_ptr<Socket>)> fn);

    void closeListenNewconnection();                //关闭对端口的监听
};