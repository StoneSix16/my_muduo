#pragma once

#include "base/Timestamp.h"
#include "base/Callback.h"
#include "base/noncopyable.h"
#include "event/Channel.h"

#include <set>
#include <vector>

class EventLoop;
class Timer;

/// @brief 定时任务通过linux提供的timerfd实现。
/// 该描述符在可以设置到期时间，并在到期时产生一个可读事件。
class TimerQueue: noncopyable
{
public: 
public:
    explicit TimerQueue(EventLoop* loop);
    /// @brief 关闭timerfd，释放timers，清除channel的资源
    /// @note 不需要析构channel，该工作会由~EventLoop完成
    ~TimerQueue();
    
    /// @brief 设置一个定时触发的回调函数。保证线程安全。
    /// 可以用别的线程调用
    /// @todo 将返回值改为timer的标识符sequence
    Timer* addTimer(TimerCallback cb,
                    Timestamp when,
                    double interval);

    /// @brief 取消一个定时器
    void cancelTimer(Timer* timer);

private:
    /// @todo 使用unique指针替代原始指针，
    /// 并使用c++14的heterogeneous comparison lookup或自定义比较函数用于map查询
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;

    /// @brief loop中添加定时器
    void addTimerInLoop(Timer* timer);

    /// @brief 取消loop中的某个定时器。
    /// 如果timer存在于timers队列中，则删除。
    /// 如果timer不存在于timers队列中，则该timer可能已到期，并正在执行回调函数。
    /// 需要将其保存在cancelingTimers队列中，供后续检查
    void cancelTimerInLoop(Timer* timer);

    /// @brief 定时器读事件触发的回调函数
    void handleRead();

    /// @brief 返回所有到期的timer，并将其从timers中移除
    std::vector<Entry> getExpired(Timestamp now);

    /// @brief 重置数组内的定时器在now+timer->interval后执行。并重置timerfd。
    void reset(const std::vector<Entry>& expired, Timestamp now);

    /// @brief 将定时器插入队列，并判断是否比当前最早到期时间更早
    bool insert(Timer* timer);

    // 注意类成员变量的初始化顺序由变量声明顺序而非初始化列表顺序决定
    // 被依赖的成员变量需要声明在前面
    
    EventLoop* loop_;           /* 所属的EventLoop */ 
    const int timerfd_;         /* linux提供的定时器描述符，记录最早的到期时间 */
    Channel timerfdChannel_;    

    // Timer list sorted by expiration
    TimerList timers_;          /* 定时器队列（内部实现是红黑树）*/
    TimerList cancelingTimers_; /* 需要被删除的定时器队列（内部实现是红黑树）*/

    bool callingExpiredTimers_; /* atomic */
};