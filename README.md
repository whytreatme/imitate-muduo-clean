# imitate-muduo —— 基于C++11/Linux的高性能Reactor网络库

仿照陈硕《Linux多线程服务端编程：使用muduo C++网络库》的设计思想，从零实现的轻量级Reactor网络库。  
该项目专注于底层事件驱动模型的构建，可作为高性能后端服务的通信中间件。

---

## ✨ 技术栈

| 类别 | 技术 |
|------|------|
| 编程语言 | C++11/14 |
| 操作系统 | Linux（支持 epoll） |
| I/O 模型 | Epoll 边缘触发（ET） + 非阻塞套接字 |
| 线程模型 | One Loop Per Thread（主从Reactor） |
| 跨线程通信 | eventfd 事件唤醒 |
| 定时器 | timerfd + 定时器回调 |
| 内存管理 | shared_ptr / weak_ptr / enable_shared_from_this |
| 构建工具 | Makefile |

---

## 🏗️ 核心架构

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                          Application Layer                          ┃
┃                       (EchoServer / 业务层)                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
                                  │
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━▼━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                       TcpServer (Framework)                       ┃
┃                                                                   ┃
┃   ┌─────────────────────────────────────────────────────────┐   ┃
┃   │  Acceptor (监听端口 5005、接收新连接、轮询分发给子线程)  │   ┃
┃   └──────────────────────────┬──────────────────────────────┘   ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━��━━━━━━━━━━━━━┛
                                  │
                 ┌────────────────┼────────────────┐
                 │                │                │
    ┌────────────▼──────┐  ┌─────▼───────────┐  ┌──▼─────────────────┐
    │  EventLoop (Main) │  │ EventLoop#1(Sub)│  │ EventLoop#N(Sub)   │
    │                   │  │                 │  │                    │
    │ ┌─────────────┐   │  │ ┌─────────────┐ │  │ ┌─────────────────┐│
    │ │   Epoll     │   │  │ │   Epoll     │ │  │ │    Epoll        ││
    │ │             │   │  │ │             │ │  │ │                 ││
    │ │ Channels    │   │  │ │ Channels    │ │  │ │  Channels       ││
    │ │ (Acceptor)  │   │  │ │(Connection) │ │  │ │ (Connection)    ││
    │ └─────────────┘   │  │ └─────────────┘ │  │ └─────────────────┘│
    │                   │  │                 │  │                    │
    │ ┌─────────────┐   │  │ ┌─────────────┐ │  │ ┌─────────────────┐│
    │ │ TimerFd     │   │  │ │ TimerFd     │ │  │ │   TimerFd       ││
    │ │ (timeout    │   │  │ │ (timeout    │ │  │ │  (timeout       ││
    │ │  check)     │   │  │ │  check)     │ │  │ │   check)        ││
    │ └─────────────┘   │  │ └─────────────┘ │  │ └─────────────────┘│
    │                   │  │                 │  │                    │
    │ eventfd唤醒       │  │ eventfd唤醒     │  │ eventfd唤醒        │
    │ (跨线程任务队列)  │  │(跨线程任务队列) │  │(跨线程任务队列)   │
    └───────────────────┘  └─────────────────┘  └────────────────────┘
         Main Thread          Worker Thread       Worker Thread
```

### 数据流向

```
Client TCP Connection
         ↓
    [Network]
         ↓
   Acceptor (Main Loop)
         ↓
   Round Robin 分配 ┐
         └─→ Sub EventLoop#1 → Epoll (ET) → 边缘触发唤醒
             ↓
         Channel → Connection
             ↓
         Buffer (非阻塞读)
             ↓
         onMessage() 业务逻辑
             ↓
         Buffer (非阻塞写)
             ↓
         网络发送回客户端
```

---

## 📦 核心模块

| 模块 | 职责 |
|------|------|
| **EventLoop** | 事件循环核心，驱动整个 Reactor 模型，管理 Channel 和定时器 |
| **Channel** | 封装文件描述符（fd）及其关注事件，分发读写/关闭/错误回调 |
| **Epoll** | 封装 epoll 系统调用，管理 Channel 的注册、更新和移除 |
| **Acceptor** | 监听端口，接受新连接，并将其分发给子 Reactor |
| **Connection** | 封装单个 TCP 连接，包含非阻塞读写 Buffer 和业务回调 |
| **Buffer** | 非阻塞读写缓冲区，支持自定义协议拆包（4字节长度头） |
| **TimerFdChannel** | 基于 timerfd 的定时器，集成到 EventLoop 中 |
| **ThreadPool** | 固定大小线程池，每个线程运行一个独立 EventLoop |

---

## ⚙️ 关键设计

### 1. One Loop Per Thread
- **主线程** 运行 `Acceptor`，负责监听新连接。
- **子线程** 各自运行独立的 `EventLoop`，通过轮询算法分配新连接，充分利用多核 CPU。

### 2. Epoll 边缘触发（ET）+ 非阻塞读写
- 采用 ET 模式配合非阻塞套接字，在 `onMessage()` 中循环读取直到返回 `EAGAIN`。
- 保证数据被完整接收的同时，最大限度地减少事件触发的次数。

### 3. 跨线程唤醒（eventfd）
- 使用 `eventfd` 实现 `queueInLoop` 机制。
- 允许工作线程安全地向 I/O 线程投递任务（如发送数据、关闭连接），避免竞态条件。

### 4. 智能指针管理生命周期
- 使用 `shared_ptr` 管理 `Connection` 对象，配合 `enable_shared_from_this` 确保回调安全。
- 使用 `weak_ptr` 在 `Channel` 中观察 `Connection`，杜绝悬空指针。

### 5. 定时器集成（timerfd）
- 基于 `timerfd` 将定时器集成到事件循环中。
- 支持连接空闲超时检测（`epollTimeout`）与周期性定时任务。

---

## 🚀 快速开始

### 环境要求
- Linux 操作系统（支持 epoll）
- C++11 及以上编译器（g++ 4.8+）
- make

### 编译

```bash
git clone https://github.com/whytreatme/imitate-muduo-clean.git
cd imitate-muduo-clean
make
```

编译完成后会生成两个可执行文件：
- `server` - 回显服务器
- `client` - 测试客户端

### 启动服务端

```bash
./server 0.0.0.0 5005 60 180
#          IP    端口  定时器周期(秒) 空闲超时(秒)
```

**参数说明：**
- `0.0.0.0` - 监听地址（0.0.0.0 表示监听所有网络接口）
- `5005` - 监听端口号
- `60` - 定时器检测周期，单位秒（定期检查超时连接）
- `180` - 连接空闲超时时间，单位秒（超过此时间无数据交互则关闭连接）

**预期输出：**
```
[main] server starting...
[EchoServer] server started on 0.0.0.0:5005, 3 worker threads
```

### 启动客户端测试（在另一个终端）

```bash
./client 127.0.0.1 5005
#          IP        端口
```

**参数说明：**
- `127.0.0.1` - 服务端地址（本机测试使用 127.0.0.1）
- `5005` - 服务端端口号

**预期输出：**
```
connect ok.
现在开始接受回复内容
recv:这是第0个矛盾存在
recv:这是第1个矛盾存在
recv:这是第2个矛盾存在
recv:这是第3个矛盾存在
recv:这是第4个矛盾存在
```

### 完整测试流程

**终端1 - 启动服务端：**
```bash
$ make                    # 编译
$ ./server 0.0.0.0 5005 60 180
[main] server starting...
[EchoServer] server started on 0.0.0.0:5005, 3 worker threads
```

**终端2 - 启动客户端：**
```bash
$ ./client 127.0.0.1 5005
connect ok.
现在开始接受回复内容
recv:这是第0个矛盾存在
recv:这是第1个矛盾存在
recv:这是第2个矛盾存在
recv:这是第3个矛盾存在
recv:这是第4个矛盾存在
```

**终端1 - 停止服务端（按 Ctrl+C）：**
```
[main] signal 2 received, stopping server...
[main] stop request finished.
[main] server main loop exited.
[main] stop thread joined, deleting server.
```

### 清理编译文件

```bash
make clean
```

---

## 🧪 性能测试

该项目支持多客户端并发连接测试：

```bash
# 终端1：启动服务端
./server 0.0.0.0 5005 60 180

# 终端2-N：启动多个客户端
./client 127.0.0.1 5005 &
./client 127.0.0.1 5005 &
./client 127.0.0.1 5005 &
```

---

## 📝 协议说明

该项目实现了简单的 **4字节长度头 + 消息体** 的自定义协议：

```
┌──────────────────────────────────────┐
│  4字节长度 (网络字节序)              │
├──────────────────────────────────────┤
│  消息体 (UTF-8 编码)                 │
└─────────────────────────────���────────┘
```

- **长度头** - 4 字节，网络字节序（Big Endian）
- **消息体** - 长度字段指定的字节数

---

## 📚 项目结构

```
imitate-muduo-clean/
├── README.md               # 项目说明文档
├── Makefile                # 编译脚本
├── src/                    # 源代码目录
│   ├── main.cpp            # 服务器主程序
│   ├── EchoServer.cpp      # 回显服务器实现
│   ├── EchoServer.h
│   ├── TcpServer.cpp       # TCP服务器框架
│   ├── TcpServer.h
│   ├── EventLoop.cpp       # 事件循环核心
│   ├── EventLoop.h
│   ├── Channel.cpp         # 通道管理
│   ├── Channel.h
│   ├── Epoll.cpp           # Epoll多路复用
│   ├── Epoll.h
│   ├── Acceptor.cpp        # 连接接收器
│   ├── Acceptor.h
│   ├── Connection.cpp      # TCP连接管理
│   ├── Connection.h
│   ├── Buffer.cpp          # 读写缓冲区
│   ├── Buffer.h
│   ├── Socket.cpp          # 套接字封装
│   ├── Socket.h
│   ├── InetAddress.cpp     # 网络地址
│   ├── InetAddress.h
│   ├── TimerFdChannel.cpp  # 定时器
│   ├── TimerFdChannel.h
│   ├── ThreadPool.cpp      # 线程池
│   ├── ThreadPool.h
│   └── Logger.h            # 日志工具
└── client/                 # 客户端目录
    └── client.cpp          # 测试客户端
```

---

## 💡 核心特性

✅ **高性能**：Epoll 边缘触发 + 非阻塞 I/O  
✅ **多线程**：主从 Reactor 模型，充分利用多核 CPU  
✅ **安全**：智能指针管理内存，避免内存泄漏  
✅ **可扩展**：易于扩展业务逻辑（如自定义协议、连接回调）  
✅ **简洁**：代码结构清晰，注释完善，适合学习  

---

## 📖 学习资源

- 《Linux 多线程服务端编程：使用 muduo C++网络库》- 陈硕
- Linux epoll 官方文档：`man epoll`
- 高性能网络编程：Reactor 模式、Non-blocking I/O、内存管理等

---

## 🤝 交流

欢迎提交 Issue 和 Pull Request！

---

**祝您使用愉快！** 🎉
