#pragma once

#include "base/Callback.h"
#include "base/noncopyable.h"
#include "base/Timestamp.h"

#include <sys/epoll.h>

class EventLoop;

/// @brief Channel封装一个文件描述符，相关的回调函数，描述符的监听器。并提供相关设置方法。
class Channel: noncopyable
{
public:
    Channel(EventLoop* eventLoop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }


    /// @brief 获取某个依赖对象的weak指针，并在需要时编程shared以防止期间对象被意外的remove
    void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; } 

    // 设置感兴趣事件类型，并在poller上注册

    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    
    // fd状态

    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    /**
     * for Poller
     * const int kNew = -1;     // fd还未被poller监视 
     * const int kAdded = 1;    // fd正被poller监视中
     * const int kDeleted = 2;  // fd被移除poller
     */ 
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop* ownerLoop() { return loop_; }
    /// @brief 从loop中移除channel
    void remove();

    // for debug
    std::string eventToString();
private:

    /// @brief 在poller中更新感兴趣的事件。
    /// 该函数在设置感兴趣事件后被调用，第一次调用会假设该channel已被添加进loop
    void update();

    /// @brief 调用回调函数
    void handleEventWithGuard(Timestamp receiveTime);

    /// @brief 事件类型
    enum eventType
    {
        kNoneEvent = 0,
        kReadEvent = EPOLLIN | EPOLLPRI,
        kWriteEvent = EPOLLOUT
    };

    EventLoop* loop_; /* fd所属的事件循环 */
    const int fd_;
    int events_; /* 感兴趣的事件类型 */
    int revents_; /* epoll/poll接收到的事件类型 */
    int index_; /* poller的注册情况 */

    std::weak_ptr<void> tie_; /* 指向依赖对象（TcpConnection）的指针，必要时转化成共享指针 */
    bool tied_;
    bool eventHandling_;
    bool addedToLoop_;

    // 相应事件的回调函数

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
