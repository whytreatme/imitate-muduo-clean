#include "Socket.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <netinet/tcp.h>

int create_non_block(){
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK ,IPPROTO_TCP);
    if (fd < 0)
    {
        printf("%s:%s:%d listen socket create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno); 
        exit(-1);
    }
    return fd;
}

Socket::Socket(int fd) : fd_(fd), isClose_(false) 
{
    
}

int Socket::fd() const
{
    return fd_;
}

std::string Socket::ip() const
{
    return ip_;
}

uint16_t Socket::port() const
{
    return port_;
}

void Socket::setreuseaddr(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));
}
void Socket::settcpnodelay(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof (optval));
}
void Socket::setreuseport(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof (optval));
}

void Socket::bind(InetAddress& servaddr){
    if(::bind(fd_, servaddr.addr(), sizeof(sockaddr)) < 0){
        perror("bind() failed"); 
        ::close(fd_); 
        exit(-1);
    }
    setIpPort(servaddr.ip() ,servaddr.port());
   
}

void Socket::setIpPort(const std::string& ip, uint16_t port)
{
    ip_ = ip;
    port_ = port;
}

void Socket::listen(int size){
    if(::listen(fd_, size) < 0){
        perror("listen() failed"); 
        ::close(fd_); 
        exit(-1);
    }
}

int Socket::accept(InetAddress& clientaddr){
    sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientsock = accept4(fd_, (sockaddr*)&peeraddr, &len, SOCK_NONBLOCK);
    clientaddr.setaddr(peeraddr);
    
    return clientsock;
}

void Socket::close()
{
    if(!isClose_)::close(fd_);
    isClose_ = true;
}

Socket::~Socket(){
    if(!isClose_)::close(fd_);
}