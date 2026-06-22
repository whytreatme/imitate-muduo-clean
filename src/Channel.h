#pragma once
#include <cstdint>
#include <functional>
#include "Socket.h"

class EventLoop;

class Channel{
private:
    int fd_ = -1;
    uint32_t events_ = 0;
    uint32_t revents_ = 0;
    bool inepoll = false;
    EventLoop &loop_ ; 
    std::function<void()> readCallback_;
    std::function<void()> errorCallback_;
    std::function<void()> closeCallback_;
    std::function<void()> writeCallback_;

public:
    Channel(int fd, EventLoop &ep);
    ~Channel();
    
    int fd();
    bool inpoll();
    bool islisten();
    void setinpoll();
    void setET();
    void setrevents(uint32_t ev);
    void enableReading();                  //注册读事件
    void disableReading();                 //注销读事件
    void enableWritting();                 //注册写事件
    void disableWritting();                //注销写事件
    void disableall();                     //取消所有事件的关注
    void remove();                         //移除Channel
    void handleEvent();
    //void newConnection(Socket *servsock);
   // void onMessage();
    void setReadcallback(std::function<void()> fn);
    void setClosecallback(std::function<void()> fn);
    void setErrorcallback(std::function<void()> fn);
    void setWritecallback(std::function<void()> fn);

    uint32_t events();
    uint32_t revents();
};