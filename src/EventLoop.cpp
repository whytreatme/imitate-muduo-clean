#include "EventLoop.h"
#include "Channel.h"
#include "errno.h"
#include <cstdio>




EventLoop::EventLoop(int timerIntervalSec) : 
                        timerIntervalSec_(timerIntervalSec),
                        timeoutClock_(*this, timerIntervalSec_),
                        wakeupfd_(eventfd(0, EFD_NONBLOCK)), 
                        wakeupChannel_(wakeupfd_, *this),
                        threadid_(-1),
                        isStop_(false),
                        taskNum_(0)
{
    timeoutClock_.setTickCallback(std::bind(&EventLoop::handleTimeout, this));
    wakeupChannel_.setReadcallback(std::bind(&EventLoop::handleWakeUp, this));
    updateChannel(&wakeupChannel_);
    wakeupChannel_.enableReading();
}

EventLoop::~EventLoop()
{
    removeChannel(&wakeupChannel_);
    ::close(wakeupfd_);
}


void EventLoop::run()
{
    //printf("EventLoop::run() thread is %ld.\n", syscall(SYS_gettid));
    {
        std::lock_guard<std::mutex> lock_(idmutex_);
        threadid_ = syscall(SYS_gettid);
    }
    printf("[EventLoop] run begin, tid=%d.\n", threadid_);
    while(!isStop_ || !(taskNum_ == 0)){
        
        std::vector<Channel*> channels = ep_.loop(10*1000);
        
        //if(channels.size() == 0) {
            //epollTimeoutCallback_(this);
            //break;
        //}
        {
            for(auto& ch : channels){
                ch->handleEvent();  
            }
        }
    }
    printf("[EventLoop] run end, tid=%d.\n", threadid_);
}

void EventLoop::stop()
{
    printf("[EventLoop] stop requested, tid=%ld.\n", syscall(SYS_gettid));
    isStop_ = true; 
    wakeUp();
}


void EventLoop::updateChannel(Channel *ch)
{
    ep_.updateChannel(ch);
}

void EventLoop::removeChannel(Channel *ch)
{
    ep_.removeChannel(ch);
}

// void EventLoop::setepollTimeoutCallback(std::function<void(EventLoop*)> fn)
// {
//     epollTimeoutCallback_ = fn;
// }

//判断当前是否处于IO线程
bool EventLoop::isinLoopThread()
{
    std::lock_guard<std::mutex> lock_(idmutex_);
    return  threadid_ == syscall(SYS_gettid);
}

//将任务塞进队列中
void EventLoop::queueInLoop(std::function<void()> fn)
{
    ++taskNum_;
    {
        std::lock_guard<std::mutex> lock_(mutex_);
        taskqueue_.push(fn);
        wakeUp();
    }
}

void EventLoop::wakeUp()
{
    uint64_t val = 1;
    ssize_t n = write(wakeupfd_, &val, sizeof(val));
    if(n < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK) return;
        if(errno == EINTR) return;
    }
}

void EventLoop::handleWakeUp()
{
    uint64_t val;
    ssize_t n = read(wakeupfd_, &val, sizeof(val));
    if(n < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK) return;
        if(errno == EINTR) return;
    }

    std::queue<std::function<void()>> tasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.swap(taskqueue_);
    }

    while (!tasks.empty()) {
        auto task = std::move(tasks.front());
        tasks.pop();
        task();
        --taskNum_;
    }
}

void EventLoop::setTimeoutCallback(std::function<void(EventLoop*)> fn)
{
    timeoutCallback_ = fn;
}

void EventLoop::handleTimeout()
{
    if(timeoutCallback_)timeoutCallback_(this);
}

