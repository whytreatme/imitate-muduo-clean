#pragma once
#include <sys/timerfd.h>
#include "Channel.h"
#include <functional>
#include <memory>

class EventLoop;

class TimerFdChannel{
private:
    int timerIntervalSec_;           //定时的周期
    
    EventLoop &loop_;
    int timerfd_;
    std::unique_ptr<Channel> timeoutChannel_;
    std::function<void()> tickCallback_;

    
public:
    TimerFdChannel(EventLoop& loop, int timerIntervalSec=60);
    ~TimerFdChannel();
    void handleTimeout();
    static int creatTimerfd(int sec = 30);
    void setTickCallback(std::function<void()> fn);
};