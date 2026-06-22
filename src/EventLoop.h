#pragma once
#include <functional>
#include "Epoll.h"
#include <memory>
#include <unistd.h>
#include <syscall.h>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include "Channel.h"
#include "TimerFdChannel.h"
#include <atomic>

class EventLoop {
private:
    int timerIntervalSec_;         //转发给TimerFdChannel

    Epoll ep_;
    TimerFdChannel  timeoutClock_;                      //定时器成员
    pid_t threadid_;                                    //循环所处的线程ID
    std::queue<std::function<void()>> taskqueue_;       //独属于IO线程的任务队列 
    std::mutex mutex_;                                  //用于保护队列被工作线程互斥访问的锁
    std::mutex idmutex_;                                //用于保护threadid_
    int wakeupfd_;                                      //用于唤醒IO线程处理等待队列的fd
    Channel wakeupChannel_;                             //唤醒队列的Channel
    
    std::atomic<bool> isStop_;                                     //用于停止事件循环的标志位
    std::atomic<int> taskNum_;                                    //还存在与队列的任务数
    std::function<void(EventLoop*)> timeoutCallback_;             //用于回调tcpserver层
    //std::function<void(EventLoop*)> epollTimeoutCallback_;
public:
    EventLoop(int timerIntervalSec=60);
    ~EventLoop();

    void run();
    void stop();                        //停止事件循环
    void updateChannel(Channel *ch);
    void removeChannel(Channel *ch);

    bool isinLoopThread();
    void queueInLoop(std::function<void()> fn);

    void wakeUp();
    void handleWakeUp();

    void handleTimeout();
    void setTimeoutCallback(std::function<void(EventLoop*)> fn);

    //void setepollTimeoutCallback(std::function<void(EventLoop*)> fn);
};