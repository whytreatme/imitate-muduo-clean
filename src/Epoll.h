#pragma once
#include <sys/epoll.h>
#include <vector>

class Channel;

class Epoll{
private:
    static const int MaxEvents = 100;
    int epollfd = -1;
    struct epoll_event events_[MaxEvents];
public:
    Epoll();
    ~Epoll();

    //void addfd(int fd, uint32_t op);
    void updateChannel(Channel* ch);
    void removeChannel(Channel *ch);
    std::vector<Channel*> loop(int out = -1);

};