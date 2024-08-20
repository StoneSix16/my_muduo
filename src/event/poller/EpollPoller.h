#pragma once

#include "event/poller/Poller.h"

#include <vector>

struct epoll_event;

class EpollPoller: public Poller
{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller();

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    static const int kInitEventListSize = 16;

    static const char* operationToString(int op);

    /// @brief 为相应的channel设置接收到的事件类型，并记录被更新的channels
    void fillActiveChannels(int numEvents, ChannelList* activeChannls) const;

    /// @brief 调用epoll_ctl 
    void update(int op, Channel* channel);

    using EventList = std::vector<epoll_event>;

    int epollFd_;
    EventList events_;
};
