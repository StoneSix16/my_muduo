#pragma once

#include "base/Callback.h"
#include "net/InetAddress.h"

#include <atomic>
#include <map>

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer: noncopyable
{
private:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
public:
    enum Option
    {
        kNoReusePort,
        kReusePort
    };
    TcpServer(EventLoop* loop, 
              const std::string& name, 
              const InetAddress& listenAddr, 
              Option option = kNoReusePort);

    ~TcpServer();

    const std::string& ipPort() const { return ipPort_; }
    const std::string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }

    /// @brief 设置用于处理io的线程数, 必须在start()前调用
    /// @param threadNum 
    /// 若为0, 则由该事件循环线程处理, 不额外创建线程.
    /// 若为1, 则由一个额外线程处理.
    /// 若为N, 则创建一个线程池处理.
    void setThreadNum(int threadNum);
    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb;}
    std::shared_ptr<EventLoopThreadPool> threadPool() {return threadPool_;}

    void start();

    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }


private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* loop_; /* acceptor 所属循环 */
    std::string name_;
    std::string ipPort_;
    std::atomic<int> started_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ThreadInitCallback threadInitCallback_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    int nextConnId;
    ConnectionMap connections_;
};


