#include "ThreadPool.h"
#include <cstdio>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include <iostream>


thpool::thpool(int nums_threads, std::string threadtype) : stop_(false), thread_type_(threadtype)
{
    for(int i = 0; i < nums_threads; i++){
      
        //使用lambda函数创建线程
        threads.emplace_back([this]{
            //1.建立thread_do类似的线程执行循环 
            while(!stop_){
                TaskItem task;
                task.fn = nullptr;
                {  //unique_lock在离开作用域自动调用析构函数，所以要有作用域限制范围
                    std::unique_lock<std::mutex> lock(mutex_);
                    tCondition.wait(lock, [this]{
                        return (stop_ == true) || !taskQueue.empty();
                    });    
                    if(stop_) break;
                    task = std::move(taskQueue.front());
                    taskQueue.pop();
                        //对taskQueue的操作完成，离开锁的作用域
                }
                printf("%s thread %ld run task: %s.\n", thread_type_.c_str(), 
                                    syscall(SYS_gettid), task.fn_name.c_str());    
                if(task.fn)  task.fn();
            }
        });
    }    
}  

void thpool::addTask(std::function<void()> task)
{
    if(!stop_) addTask("unamed", std::move(task));
}

void thpool::addTask(std::string name, std::function<void()> task)
{ //在Pithikos的实现里，要把函数对象指针和参数指针一起传给任务，但是C++
  //有包装器，可以连参数也一起绑定了
    if(stop_) return;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        taskQueue.push(TaskItem{std::move(name), std::move(task)});     //涉及任务队列操作一定要有同步机制防止竞态
    } 
    tCondition.notify_one();     //由条件变量通知阻塞的线程
}  

size_t thpool::size() const
{
    return threads.size();
}

void thpool::stop()
{
    stop_ = true;
    tCondition.notify_all();
}

void thpool::join()
{
    for(auto &th : threads){
        if(th.joinable())  th.join();
    }
}

void thpool::stopAndjoin()
{
    printf("[%s] stopAndjoin begin.\n", thread_type_.c_str());
    stop();
    join();    
    printf("[%s] stopAndjoin end.\n", thread_type_.c_str());
}

thpool::~thpool()
{
    stop();
    join();
}  

