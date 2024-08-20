#include "event/EventLoop.h"
#include "event/poller/Poller.h"
#include "logger/Logging.h"
#include "base/CurrentThread.h"

#include <mutex>
#include <vector>
#include <assert.h>
#include <sys/eventfd.h>

namespace Utils
{
__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

void assertInLoopThread(EventLoop* loop)
{
    if(!loop->isInLoopThread())
    {
        LOG_FATAL << "Utils::assertInLoopThread - EventLoop " << loop
        << " was created in threadId_ = " << loop->threadId()
        << ", current thread id = " <<  CurrentThread::tid();
    }
}

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_ERROR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

}

EventLoop::EventLoop():
    looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    iteration_(0),
    threadId_(CurrentThread::tid()),
    pollReturnTime_(),
    wakeupFd_(Utils::createEventfd()),
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    activeChannels_(),
    currentActiveChannel_(nullptr)
{
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    if(Utils::t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << Utils::t_loopInThisThread
              << " exists in this thread " << threadId_;
    }
    else
    {
        Utils::t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // poller会持续监视wakeupChannel的状态
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
        << " destructs in thread " << CurrentThread::tid();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    Utils::t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    assert(!looping_);
    Utils::assertInLoopThread(this);
    looping_ = true;
    LOG_DEBUG << "EventLoop " << this << " start looping";

    while(!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(Utils::kPollTimeMs, &activeChannels_);
        ++iteration_;

        /// 处理channel
        eventHandling_ = true;
        for(Channel* channel: activeChannels_)
        {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        doPendingFunctors();
    }

    LOG_DEBUG << "EventLoop " << this << " stop looping";
    looping_ = false;
    /// 在退出循环时重置quit。如果在进入循环前调用quit()，则跳过这次循环。
    quit_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

EventLoop *EventLoop::getLoopOfCurrentThread()
{
    return Utils::t_loopInThisThread;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.swap(functors);
    }

    for(const Functor& func: functors)
    {
        func();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    Utils::assertInLoopThread(this);
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    Utils::assertInLoopThread(this);
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    Utils::assertInLoopThread(this);
    return poller_->hasChannel(channel);
}

Timer *EventLoop::runAt(Timestamp time, TimerCallback cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

Timer *EventLoop::runAfter(double delay, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

Timer *EventLoop::runEvery(double interval, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(Timer *timer)
{
    timerQueue_->cancelTimer(timer);
}
