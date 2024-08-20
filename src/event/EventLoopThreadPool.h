#pragma once

#include "event/EventLoop.h"
#include "event/EventLoopThread.h"

#include <vector>

class EventLoopThreadPool: noncopyable
{
public:
    using EventLoopThreadInitCallback = EventLoopThread::EventLoopThreadInitCallback;
    EventLoopThreadPool(EventLoop* baseLoop, const std::string& name = std::string());
    ~EventLoopThreadPool();

    void setNumThread(int num);

    /// @brief 开启指定数量的循环线程。只能由线程池的创建者调用
    /// @param cb 循环线程调用前需要执行的回调函数
    void start(const EventLoopThreadInitCallback& cb = EventLoopThreadInitCallback());

    EventLoop* getNextLoop();
    std::vector<EventLoop*> getAllLoops();

    /// @brief 对同一个hash码，返回同一个loop
    EventLoop* getLoopForHash(size_t hashCode);

    const std::string& name() const {return name_;}
    bool started() const {return started_; }
private:
    EventLoop* baseLoop_;
    std::string name_;
    bool started_;
    int numThread_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};
