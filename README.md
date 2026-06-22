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
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━��
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
git clone https://github.com/你的用户名/imitate-muduo.git
cd imitate-muduo
make
```

### 启动服务端
```bash
./server 0.0.0.0 5005 60 180
#         IP    端口  定时器周期(秒) 空闲超时(秒)
```

**参数说明：**
- `0.0.0.0` - 监听地址（0.0.0.0 表示监听所有网络接口）
- `5005` - 监听端口号
- `60` - 定时器检测周期，单位秒（定期检查超时连接）
- `180` - 连接空闲超时时间，单位秒（超过此时间无数据交互则关闭连接）

