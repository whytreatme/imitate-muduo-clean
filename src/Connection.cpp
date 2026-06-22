#include "Connection.h"
#include <unistd.h>
#include <cstring>
#include "Logger.h"



Connection::Connection(EventLoop &loop, std::unique_ptr<Socket> clientsock, int idleTimeoutSec) 
          : loop_(loop), clientsock_(std::move(clientsock)), isDisconnect(false),
            clientChannel_(new Channel(clientsock_->fd(), loop_)),
            idleTimeoutSec_(idleTimeoutSec)
{
    LOG("Connection::Ctor(fd=%d) - Creating Connection object.", clientsock_->fd());
    //printf("现在调用Connection的构造函数！\n");
    lastActive_ = std::chrono::steady_clock::now();
    clientChannel_->setReadcallback(std::bind(&Connection::onMessage, this));
    clientChannel_->setClosecallback(std::bind(&Connection::closeCallback, this));
    clientChannel_->setErrorcallback(std::bind(&Connection::errorCallback, this));
    clientChannel_->setWritecallback(std::bind(&Connection::writeCallback, this));
    clientChannel_->setET();
    LOG("Connection::Ctor(fd=%d) - ENABLING READING. The race condition starts NOW!", clientsock_->fd());
    
}

Connection::~Connection()
{
    //LOG("Connection::Dtor(fd=%d) - Destroying Connection object.", fd());
    //delete clientChannel_;
    //delete clientsock_;
}

int Connection::fd() const
{
    return clientsock_->fd();
}

std::string Connection::ip() const
{
    return clientsock_->ip();
}

uint16_t Connection::port() const
{
    return clientsock_->port();
}


void Connection::errorCallback()
{
    // printf("client(eventfd=%d) error.\n",fd());
    // close(fd());            // 关闭客户端的fd。
    LOG("Connection::errorCallback(fd=%d) - Channel reports an error. Invoking server callback.", fd());
    if(isDisconnect) return;
    isDisconnect = true;
    clientChannel_->remove();
    clientsock_->close();
    auto self = shared_from_this();
    errorCallback_(self);
}

void Connection::closeCallback()
{
    LOG("Connection::closeCallback(fd=%d) - Channel reports connection closed. Invoking server callback.", fd());
    // std::cout << "client(eventfd= " << fd() <<") disconnected.\n";
    // close(fd());
    if(isDisconnect) return;
    isDisconnect = true;
    clientChannel_->remove();
    clientsock_->close();
    auto self = shared_from_this();
    closeCallback_(self);
}

void Connection::setCloseCallback(std::function<void(spConnection)> fn)
{
    closeCallback_ = fn;
}

void Connection::setErrorCallback(std::function<void(spConnection)> fn)
{
    errorCallback_ = fn;
}

void Connection::setonMessageCallback(std::function<void(spConnection, std::string&)> fn)
{
    onMessageCallback_ = fn;
}

void Connection::setsendCompleteCallback(std::function<void(spConnection)> fn)
{
    sendCompleteCallback_ = fn;
}

void Connection::onMessage(){
    LOG("Connection::onMessage(fd=%d) - Readable event triggered.", fd());
    {
        std::lock_guard<std::mutex> lA_lock(mutex_);
        lastActive_ = std::chrono::steady_clock::now();
    }
        char buffer[1024];
    while(true){
        memset(buffer, 0, sizeof(buffer));
        ssize_t nread = recv(fd(), buffer, sizeof(buffer) ,0);
        //全部数据已经发送完毕
        if(nread < 0) {
            if((errno == EAGAIN) ||(errno == EWOULDBLOCK)){ //数据已传输完成
                LOG("Connection::onMessage(fd=%d) - All data read from socket. Processing buffer.", fd());
                while(true){
                    
                    std::string message;
                    inputbuffer_.getText(message);
                    if(message.size() == 0) break;
                    ///////////////////////////////////////////////////////
                    //printf("message (eventfd=%d):%s\n", fd(), message.c_str());
                    LOG("Connection::onMessage(fd=%d) - Extracted message, size=%zu. About to invoke onMessageCallback_.", fd(), message.length());
                    onMessageCallback_(shared_from_this(), message);
                }
                break;
            }
            if(errno == EINTR) continue;
            else {
                // ！！！ 其他所有未知的负数错误 (如 ECONNRESET) ！！！
                // 视为客户端异常断开，绝对不能当作长度去 append！
                LOG("Connection::onMessage(fd=%d) - recv error, errno=%d", fd(), errno);
                errorCallback(); // <--- 调用错误回调，触发清理流程
                break;
            }
        }

        else if(nread == 0){
            LOG("Connection::onMessage(fd=%d) - Peer has disconnected (recv returned 0).", fd());
            // std::cout << "2client(eventfd= " << fd_ <<") disconnected.\n";
            // close(fd_);
            closeCallback();
            break;
        }
        else{
          inputbuffer_.append(buffer, nread);
        }
    }
}

//使用发送缓冲区发送数据
void Connection::send(const char *data, size_t size)
{
    if(isDisconnect){
        LOG("Connection::send(fd=%d) - Connection is already disconnected, send ignored.", fd());
        return;
    }
    {
        std::lock_guard<std::mutex> lA_lock(mutex_);
        lastActive_ = std::chrono::steady_clock::now();
    }
    std::shared_ptr<std::string> message_(new std::string(data, size));
    if(loop_.isinLoopThread()){
        sendInLoop(message_);
    }
    else{
        loop_.queueInLoop(std::bind(&Connection::sendInLoop, shared_from_this(), message_));
    }
    
}

//只允许IO线程操作内核态的缓冲区
void Connection::sendInLoop(std::shared_ptr<std::string> message)
{
    if(isDisconnect) return;
    outputbuffer_.appendWithhead(message->data(), message->size());       //把数据读入缓冲区
    clientChannel_->enableWritting();
}

//写事件到达写入缓冲区
void Connection::writeCallback()
{
    if(isDisconnect) return;
    size_t writen = ::send(fd(), outputbuffer_.data(), outputbuffer_.size(), 0);
    if(writen > 0) outputbuffer_.erase(0, writen);
    
    //如果用户定义的缓冲区没有数据了，就注销写事件的关注
    if(outputbuffer_.size() == 0) {
        clientChannel_->disableWritting();
        sendCompleteCallback_(shared_from_this());
    }
}

void Connection::ConnectEstablished()
{
    LOG("Connection::connectEstablished(fd=%d) - Connection is fully set up. ENABLING READING now.", fd());
    clientChannel_->enableReading();
}

bool Connection::isConnect() const
{
    return !isDisconnect;
}

bool Connection::isIdle() 
{
    std::lock_guard<std::mutex> lA_lock(mutex_);
    return (std::chrono::steady_clock::now() - lastActive_) > std::chrono::seconds(idleTimeoutSec_);
}

EventLoop* Connection::getloop() const
{
    return &loop_;
}

void Connection::close()
{
    if(loop_.isinLoopThread()){
        closeInLoop();
    }
    else{
        loop_.queueInLoop(std::bind(&Connection::closeInLoop, shared_from_this()));
    }
}

void Connection::closeInLoop()
{
    closeCallback();
    
}