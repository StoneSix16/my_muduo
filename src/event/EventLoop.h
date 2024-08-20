#pragma once

#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include "base/CurrentThread.h"
#include "timer/TimerQueue.h"

#include <functional>
#include <vector>
#include <atomic>
#include <mutex>

namespace Utils
{
    void assertInLoopThread(EventLoop* loop);
}


class Channel;
class Poller
;
class EventLoop: noncopyable
{
public:
    using Functor = std::function<void()>;
    EventLoop(/* args */);
    ~EventLoop();

    /// @brief 开启循环。必须由创建该事件循环对象的线程调用
    void loop();
    
    /// @brief 退出循环。
    void quit();

    Timestamp pollReturnTime() const {return pollReturnTime_;}
    pid_t threadId() const {return threadId_; }

    /// @brief 在循环线程中调用函数。该操作唤醒循环并调用函数
    /// 如果当前线程是创建该循环的线程，则回调函数在该次调用中执行
    /// 可以被其他线程调用
    void runInLoop(Functor cb);

    /// @brief 将函数加入队列，唤醒对应循环线程并执行
    /// 可以被其他线程调用
    void queueInLoop(Functor cb);

    // channel的相关操作，通过调用poller下的相关方法实现
    
    /// @brief 通过对wakeupfd执行写操作，唤醒线程
    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // timer的相关操作，向队列中添加定时器
    Timer* runAt(Timestamp time, TimerCallback cb);
    Timer* runAfter(double delay, TimerCallback cb);
    Timer* runEvery(double interval, TimerCallback cb);
    void cancel(Timer* timer);

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    static EventLoop* getLoopOfCurrentThread(); 
private:
    /// @brief wakupFd触发可读事件后，调用该函数读取以避免重复触发
    void handleRead();
    /// @brief 执行待执行的回调函数
    void doPendingFunctors();
    
    using ChannelList = std::vector<Channel*>;

    bool looping_;
    std::atomic<bool> quit_; /* 表示是否有线程调用了quit()。多线程调用quit()可能会同时修改quit */
    bool eventHandling_;
    bool callingPendingFunctors_;

    int64_t iteration_;
    const pid_t threadId_;
    Timestamp pollReturnTime_;

    int wakeupFd_;
    /// @note 需要注意声明的顺序。
    /// 由于channel在析构时会检查自己是否在对应poller中，故poller需要在channel后被析构
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::unique_ptr<TimerQueue> timerQueue_;

    ChannelList activeChannels_;    /* 被激活的channels */
    Channel* currentActiveChannel_;

    mutable std::mutex mutex_;  /* pendingFunctor的互斥锁 */
    std::vector<Functor> pendingFunctors_;
};

