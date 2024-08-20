#include "timer/TimerQueue.h"
#include "timer/Timer.h"
#include "logger/Logging.h"
#include "event/EventLoop.h"

#include <iterator>
#include <vector>
#include <sys/timerfd.h>
#include <unistd.h>

namespace Utils
{
    int createTimerfd()
    {
        /**
         * CLOCK_MONOTONIC：绝对时间
         * TFD_NONBLOCK：非阻塞
         */
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                        TFD_NONBLOCK | TFD_CLOEXEC);
        if (timerfd < 0)
        {
            LOG_ERROR << "Utils::createTimerfd";
        }
        return timerfd;
    }

    // struct timespec
    // {
    //         __time_t tv_sec;		/* Seconds.  */
    //         __syscall_slong_t tv_nsec;	/* Nanoseconds.  */
    // };
    // struct itimerspec
    // {
    //     struct timespec it_interval;
    //     struct timespec it_value;
    // };
    struct timespec howMuchTimeFromNow(Timestamp when)
    {
        int64_t microseconds = when.microSecondsSinceEpoch()
                                - Timestamp::now().microSecondsSinceEpoch();
        if (microseconds < 100)
        {
            microseconds = 100;
        }
        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(
            microseconds / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(
            (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
        return ts;
    }

    void resetTimerfd(int timerfd, Timestamp expiration)
    {   
        itimerspec oldValue, newValue;
        memset(&oldValue, 0, sizeof(oldValue));
        memset(&newValue, 0, sizeof(newValue));
        newValue.it_value = howMuchTimeFromNow(expiration);
        int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
        if(ret)
        {
            LOG_ERROR << "Utils::resetTimerfd";
        }
    }

    void readTimerfd(int timerfd)
    {
        uint64_t howmany;
        ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
        if (n != sizeof howmany)
        {
            LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
        }
    }
}

TimerQueue::TimerQueue(EventLoop *loop):
    loop_(loop),
    timerfd_(Utils::createTimerfd()),
    timerfdChannel_(loop, timerfd_),
    timers_(),
    callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for(const Entry& entry: timers_)
    {
        delete entry.second;
    }
}

Timer* TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(
        std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return timer;
}

void TimerQueue::cancelTimer(Timer *timer)
{
    loop_->runInLoop(
        std::bind(&TimerQueue::cancelTimerInLoop, this, timer));
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    Utils::assertInLoopThread(loop_);
    bool earliestChanged = insert(timer);
    if(earliestChanged)
    {
        Utils::resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancelTimerInLoop(Timer *timer)
{
    Utils::assertInLoopThread(loop_);
    Timestamp expiration = timer->expiration();
    auto ite = timers_.find(Entry(expiration, timer));
    if(ite != timers_.end()) // 定时器存在于队列中
    {
        timers_.erase(ite);
        delete ite->second;
    }
    /// @todo 是否真的需要cancelingTimers？
    /// 由于cancel和expire的处理都在同个线程进行，理论上不需要cancelingTimer进行同步。
    /// callingExpiredTimers_在该函数里不应该为true
    else if(callingExpiredTimers_)
    {
        LOG_INFO << "TimerQueue::cancelTimerInLoop inserts a timer into cancelingTimers_";
        cancelingTimers_.insert(Entry(expiration, timer));
    }
}

void TimerQueue::handleRead()
{
    Utils::assertInLoopThread(loop_);

    Timestamp now = Timestamp::now();
    Utils::readTimerfd(timerfd_);
    std::vector<Entry> expired = getExpired(now);

    /// @todo 是否真的需要callingExpiredTimers_？
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for(const Entry& entry: expired)
    {
        entry.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    auto ite = timers_.lower_bound(Entry(now, reinterpret_cast<Timer*>(UINTPTR_MAX)));
    std::copy(timers_.begin(), ite, std::back_inserter(expired));
    timers_.erase(timers_.begin(), ite);
    
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    for(const Entry& entry: expired)
    {
        /// @todo canceling与delete
        if(entry.second->repeat()
        && cancelingTimers_.find(entry) == cancelingTimers_.end())
        {
            entry.second->restart(now);
            insert(entry.second);
        }
        else
        {
            delete entry.second;
        }
    }

    if(!timers_.empty())
    {
        Utils::resetTimerfd(timerfd_, (timers_.begin()->second)->expiration());
    }
}

bool TimerQueue::insert(Timer *timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    auto ite = timers_.begin();
    if(ite == timers_.end() || when < ite->first)
    {
        earliestChanged = true;
    }
    timers_.insert(Entry(when, timer));
    return earliestChanged;
}
