/*
* 我的回显服务器
*/
#include <iostream>
#include <cstdio>
#include <csignal>
#include <thread>
#include <pthread.h>
#include "EchoServer.h"
using namespace std;

int main(int argc, char *argv[]){
    if(argc != 5){
        cout << "Usage " << argv[0] << " ip port timerIntervalSec idleTimeoutSec\n";
        cout << "./main 0.0.0.0 5005 60 180\n\n";
        return -1;
    }

    // 1) 阻塞信号，交给 sigwait 处理
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);

   
     EchoServer *echoserver = new EchoServer(argv[1], atoi(argv[2]), 3, 5, atoi(argv[3]), atoi(argv[4]));
   // 2) 让服务器在独立线程运行
    std::thread serverThread([&]{
        int sig = 0;
        sigwait(&set, &sig);
        printf("[main] signal %d received, stopping server...\n", sig);
        echoserver->Stop();
        printf("[main] stop request finished.\n");
    });
    printf("[main] server starting...\n");
    echoserver->Start();
    printf("[main] server main loop exited.\n");

   
    serverThread.join();
    printf("[main] stop thread joined, deleting server.\n");
    delete echoserver;

    return 0;
}