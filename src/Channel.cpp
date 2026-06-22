#include "Channel.h"
#include <sys/epoll.h>
#include <iostream>
#include <unistd.h>
#include "EventLoop.h"
#include "InetAddress.h"
//#include "cstring"
#include "Socket.h"




Channel::Channel(int fd, EventLoop &loop) : fd_(fd), loop_(loop)
{
    //printf("我现在在Channel的构造函数里！\n");
}  

int Channel::fd(){
    return this->fd_;
}

bool Channel::inpoll(){
    return this->inepoll;
}

void Channel::setinpoll(){
    this->inepoll = true;
}

void Channel::setET(){
    this->events_ |= EPOLLET;
}

void Channel::setrevents(uint32_t ev) {
    this->revents_ = ev;
}

uint32_t Channel::events(){
    return this->events_;
}

uint32_t Channel::revents(){
    return this->revents_;
}

//取消所有事件的关注
void Channel::disableall()
{
    this->events_ = 0;
} 

//移除Channel
void Channel::remove()
{
    disableall();
    loop_.removeChannel(this);
}

Channel::~Channel(){}

void Channel::enableReading(){
    this->events_ |= EPOLLIN;
    // this->ep_->updateChannel(this);
    loop_.updateChannel(this);
}

void Channel::disableReading()
{
    this->events_ &= ~EPOLLIN;
    loop_.updateChannel(this);
}

void Channel::enableWritting()
{
    this->events_ |= EPOLLOUT;
    loop_.updateChannel(this);
}

void Channel::disableWritting()
{
    this->events_ &= ~EPOLLOUT;
    loop_.updateChannel(this);
}

void Channel::handleEvent(){
    if(revents_ & EPOLLRDHUP){
        // std::cout << "1client(eventfd= " << fd_ <<") disconnected.\n";
        // close(fd_);
        closeCallback_();
    }
    else if(revents_ & (EPOLLIN | EPOLLPRI)){
        //printf("我现在在Channel::handleEvent里！\n");
        readCallback_();
    } 
     else if (revents_ & EPOLLOUT)                  // 有数据需要写，暂时没有代码，以后再说。
    {
        writeCallback_();
    }
    else                                                                   // 其它事件，都视为错误。
    {
        // printf("client(eventfd=%d) error.\n",fd_);
        // close(fd_);            // 关闭客户端的fd。
        errorCallback_();
    }

}



void Channel::setClosecallback(std::function<void()> fn)
{
    closeCallback_ = fn;
}

void Channel::setErrorcallback(std::function<void()> fn)
{
    errorCallback_ = fn;
}

void Channel::setReadcallback(std::function<void()> fn)
{
    readCallback_ = fn;
}

void Channel::setWritecallback(std::function<void()> fn)
{
    writeCallback_ = fn;
}