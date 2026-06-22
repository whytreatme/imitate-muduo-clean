all:client main

client:client.cpp
	g++ -g -o client client.cpp
main:main.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp\
		EchoServer.cpp ThreadPool.cpp Logger.h TimerFdChannel.cpp
	g++ -g -o main main.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp\
       		  Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp TimerFdChannel.cpp -lpthread

clean:
	rm -f client main
