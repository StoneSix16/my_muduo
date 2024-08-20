#include "event/EventLoopThread.h"
#include "EventLoopThread.h"

#include <assert.h>

EventLoopThread::EventLoopThread(EventLoopThreadInitCallback cb, const std::string &name):
    loop_(nullptr),
    thread_(std::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    initCallback_(cb)
{
    sem_init(&sem_, false, 0);
}

EventLoopThread::~EventLoopThread()
{
    // 如果析构时，loop被初始化但未开启循环，则loop会直接退出循环
    if(loop_)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::start()
{
    assert(!thread_.started());
    thread_.start();

    sem_wait(&sem_);
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if(initCallback_)
    {
        initCallback_(&loop);
    }

    loop_ = &loop;
    sem_post(&sem_);
    
    loop_->loop();

    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
