#pragma once

#include "event/EventLoop.h"
#include "base/Timestamp.h"
#include "base/noncopyable.h"

#include <unordered_map>
#include <vector>

class Channel;

class Poller: noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;
    
    Poller(EventLoop* loop);
    virtual ~Poller();

    /// @brief 调用poll wait监听事件，并通知channel。
    /// 只能由loop线程调用
    /// @return 事件发生的时间戳
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    /// @brief 更新channel的感兴趣事件。
    /// 只能由loop线程调用
    virtual void updateChannel(Channel* channel) = 0;

    /// @brief 移除channel，用于析构函数。
    /// 只能由loop线程调用
    virtual void removeChannel(Channel* channel) = 0;

    /// @brief 查找是否注册了对应的channel
    virtual bool hasChannel(Channel* channel) const;

    /// @brief 设置默认的poller（Epoller）
    static Poller* newDefaultPoller(EventLoop* loop);

    void assertInLoopThread() const { Utils::assertInLoopThread(loop_); }

protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop* loop_;
};
