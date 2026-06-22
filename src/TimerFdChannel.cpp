#include "TimerFdChannel.h"
#include <iostream>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include "EventLoop.h"

int TimerFdChannel::creatTimerfd(int sec) 
{
    int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (tfd < 0) {
        std::cerr << "timerfd_create failed: " << std::strerror(errno) << "\n";
        return -1;
    }

    itimerspec its{};             // 零初始化
    its.it_value.tv_sec = sec;      // 首次触发
    its.it_interval.tv_sec = sec;   // 周期触发
    if (::timerfd_settime(tfd, 0, &its, nullptr) < 0) {
        std::cerr << "timerfd_settime failed: " << std::strerror(errno) << "\n";
        ::close(tfd);
        return -1;
    }
    return tfd;
}

TimerFdChannel::TimerFdChannel(EventLoop& loop, int timerIntervalSec) 
               : timerIntervalSec_(timerIntervalSec),
                 loop_(loop), timerfd_(creatTimerfd(timerIntervalSec_))
              
{
    if(timerfd_ >= 0){
        timeoutChannel_ = std::make_unique<Channel>(timerfd_, loop_);
        timeoutChannel_->setReadcallback(std::bind(&TimerFdChannel::handleTimeout, this));
        loop_.updateChannel(timeoutChannel_.get());
        timeoutChannel_->enableReading();
    }
}

TimerFdChannel::~TimerFdChannel()
{
    if(timeoutChannel_) timeoutChannel_->remove();
    if(timerfd_ != -1)::close(timerfd_);
}

void TimerFdChannel::setTickCallback(std::function<void()> fn)
{
    tickCallback_ = fn;
}

void TimerFdChannel::handleTimeout()
{
    //读取定时操作
    std::uint64_t expirations = 0;
    const ssize_t r = ::read(timerfd_, &expirations, sizeof(expirations)); // 清空 timerfd
    if (r < 0 ) {
        if(errno == EINTR) return;
        else if(errno == EAGAIN) return;
        else {
            std::cerr << "read failed: " << std::strerror(errno) << "\n";
            return;
        }
    }
    if(r != sizeof(expirations)){
        std::cerr << "read failed: Not completement read!" << "\n";
        return;
    }
    
    if(tickCallback_) tickCallback_();
}