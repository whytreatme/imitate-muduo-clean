#include "InetAddress.h" 

/*class InetAddress{
    private:
        struct sockaddr_in addr_;
    public:
        InetAddress(const std::string ip, uint16_t port);
        InetAddress(const struct sockaddr_in addr);
        const char* ip();
        const uint16_t port();
        const struct sockaddr* addr(); 

};*/ 

//设置基础的sockaddr_in属性
InetAddress::InetAddress(const std::string &ip, uint16_t port){
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}  

//也可以直接通过sockaddr_in构造
InetAddress::InetAddress(const struct sockaddr_in addr) : addr_(addr)
{

}

InetAddress::InetAddress(){}

const char*InetAddress::ip() const{
    return inet_ntoa(addr_.sin_addr);
}

uint16_t InetAddress::port() const{
    return ntohs(addr_.sin_port);
}

const struct sockaddr* InetAddress::addr() const{
    return (struct sockaddr*)&addr_;
} 

void InetAddress::setaddr(const InetAddress& new_){
    addr_ = new_.addr_;
}