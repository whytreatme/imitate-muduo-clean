#include "Buffer.h"
#include <cstring>
#include <arpa/inet.h>


Buffer::Buffer()
{

}

Buffer::~Buffer()
{

}

void Buffer::append(const char *data, size_t size)
{
    buf_.append(data, size);
}

size_t Buffer::size()
{
    return buf_.size();
}

const char * Buffer::data()
{
    return buf_.data();
}

 //擦除从pos开始的nn个字节
void Buffer::erase(size_t pos, size_t nn)
{
    buf_.erase(pos, nn);
}   

void Buffer::clear()
{
    buf_.clear();
}  

//把数据头部和数据追加到buf_中
void Buffer::appendWithhead(const char *data, size_t size)     
{
    uint32_t len = size;         
    uint32_t nlen = htonl(len);       //把报文头部换网络字节序（数字必须）
    buf_.append((char*)&nlen, 4);
    buf_.append(data, size);
}

//自动把报文提取出来
void Buffer::getText(std::string& msg)                             
{
    if(buf_.size() <= 4) return;                        //连头部也不够
    uint32_t nlen, len;
    memcpy(&nlen, buf_.data(), 4);        
    len = ntohl(nlen);
    if(buf_.size() < len + 4) return;                  //没发完完整报文
    msg.append(buf_.data() + 4, len); 
    buf_.erase(0, len + 4);                           //取出报文后就要把buf_中的报文去掉
}