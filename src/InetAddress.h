#pragma once
#include <arpa/inet.h>
#include <string>

class InetAddress{
    private:
        struct sockaddr_in addr_;
    public:
        InetAddress(const std::string &ip, uint16_t port);
        InetAddress(const struct sockaddr_in addr);
        InetAddress();
        const char* ip() const;
        uint16_t port() const;
        const struct sockaddr* addr() const; 
        void setaddr(const InetAddress&);

};