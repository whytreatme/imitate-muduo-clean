CXX = g++
CXXFLAGS = -g -std=c++11
LDFLAGS = -lpthread

# Source files in src/ directory
SRC_DIR = src
CLIENT_DIR = client

SERVER_SOURCES = $(SRC_DIR)/main.cpp \
                 $(SRC_DIR)/InetAddress.cpp \
                 $(SRC_DIR)/Socket.cpp \
                 $(SRC_DIR)/Epoll.cpp \
                 $(SRC_DIR)/Channel.cpp \
                 $(SRC_DIR)/EventLoop.cpp \
                 $(SRC_DIR)/TcpServer.cpp \
                 $(SRC_DIR)/Acceptor.cpp \
                 $(SRC_DIR)/Connection.cpp \
                 $(SRC_DIR)/Buffer.cpp \
                 $(SRC_DIR)/EchoServer.cpp \
                 $(SRC_DIR)/ThreadPool.cpp \
                 $(SRC_DIR)/TimerFdChannel.cpp

CLIENT_SOURCES = $(CLIENT_DIR)/client.cpp

TARGETS = server client

.PHONY: all clean

all: $(TARGETS)

server: $(SERVER_SOURCES)
	$(CXX) $(CXXFLAGS) -o server $(SERVER_SOURCES) $(LDFLAGS)

client: $(CLIENT_SOURCES)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT_SOURCES)

clean:
	rm -f server client
