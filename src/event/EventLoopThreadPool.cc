#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <assert.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &name):
    baseLoop_(baseLoop),
    name_(name),
    started_(false),
    numThread_(0),
    next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::setNumThread(int num)
{
    assert(!started_);
    assert(threads_.size() == 0);
    assert(loops_.size() == 0);
    numThread_ = num;
}

void EventLoopThreadPool::start(const EventLoopThreadInitCallback& cb)
{
    assert(!started_);
    Utils::assertInLoopThread(baseLoop_);

    loops_.reserve(numThread_);
    for(int i = 0; i < numThread_; i++)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        threads_.emplace_back(new EventLoopThread(cb, buf));
        loops_[i] = threads_[i]->start();
    }

    if(numThread_ == 0 && cb)
    {
        cb(baseLoop_);
    }
    started_ = true;
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    Utils::assertInLoopThread(baseLoop_);
    assert(started_);
    EventLoop* ret = baseLoop_;

    if(numThread_ > 0)
    {
        ret = loops_[next_];
        next_ = (next_+1)%numThread_;
    }
    return ret;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    Utils::assertInLoopThread(baseLoop_);
    assert(started_);
    if(numThread_ > 0)
    {
        return loops_;
    }
    else
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
}

EventLoop *EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
    Utils::assertInLoopThread(baseLoop_);
    EventLoop* loop = baseLoop_;

    if (!loops_.empty())
    {
        loop = loops_[hashCode % numThread_];
    }
    return loop;
}
