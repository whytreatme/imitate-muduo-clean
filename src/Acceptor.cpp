#include "Acceptor.h"
#include "Logger.h"



Acceptor::Acceptor(EventLoop &loop, const std::string &ip, uint16_t port) : 
          loop_(loop),  servsock_(create_non_block()), acceptChannel_(servsock_.fd(), loop_)
{
    servsock_.setreuseaddr(true);
    servsock_.setreuseport(true);
   
    InetAddress servaddr(ip, port);
    servsock_.bind(servaddr);
    servsock_.listen();
      
    acceptChannel_.setReadcallback(std::bind(&Acceptor::newConnection, this));
    acceptChannel_.enableReading();
}

Acceptor::~Acceptor()
{
    //delete servsock_;
    //delete acceptChannel_;
}

//#include "Connection.h"
void Acceptor::newConnection(){
    InetAddress clientaddr;
    std::unique_ptr<Socket> clientsock(new Socket(servsock_.accept(clientaddr)));
    clientsock->settcpnodelay(true);
    clientsock->setIpPort(clientaddr.ip(), clientaddr.port());
    //printf ("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());
 
    //Connection* conn = new Connection(loop_, clientsock);
    LOG("Acceptor::newConnection - Accepted new client fd=%d, ip=%s, port=%d", clientsock->fd(), clientaddr.ip(), clientaddr.port());
    newConnectioncb_(std::move(clientsock));
}

void Acceptor::setnewConnection(std::function<void(std::unique_ptr<Socket>)> fn)
{
    newConnectioncb_ = fn;
}


void Acceptor::closeListenNewconnection()
{
    acceptChannel_.remove();
    servsock_.close();
}