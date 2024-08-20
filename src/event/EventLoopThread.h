#pragma once

#include "base/Callback.h"
#include "base/Thread.h"
#include "event/EventLoop.h"

#include <semaphore.h>

class EventLoopThread: noncopyable
{
public:
    using EventLoopThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThread(EventLoopThreadInitCallback cb = EventLoopThreadInitCallback(),
                    const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* start();
private:
    void threadFunc();

    EventLoop* loop_;
    Thread thread_;
    std::mutex mutex_;
    sem_t sem_;
    EventLoopThreadInitCallback initCallback_;
};