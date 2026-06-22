#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <atomic>

class thpool{
private:
    struct TaskItem{
        std::string fn_name;
        std::function<void()> fn;
    };

    std::vector<std::thread> threads;                    //建立线程池
    std::queue<TaskItem> taskQueue;                      //建立任务队列  
    std::mutex mutex_;                                   //设置锁保护任务队列 
    std::condition_variable  tCondition;                 //保证同步的条件变量  
    std::atomic<bool> stop_;                                 //结束标志位

    const std::string thread_type_;                      //定义线程种类"IO"、"WORKS"

public:  
    thpool(int nums_threads, std::string threadtype);
    ~thpool();
    void addTask(std::function<void()> task);            //兼容旧接口，名字为"unamed"
    void addTask(std::string name, std::function<void()> task);
    size_t size() const;
    void stop();                                        //设置暂停标志位
    void join();                                        //显式等待进程退出
    void stopAndjoin();                                 //彻底退出的接口
};