#include "event/poller/EpollPoller.h"
#include "logger/Logging.h"

#include <sys/epoll.h>
#include <assert.h>

// typedef union epoll_data
// {
//   void *ptr;
//   int fd;
//   uint32_t u32;
//   uint64_t u64;
// } epoll_data_t;

// struct epoll_event
// {
//   uint32_t events;	/* Epoll events */
//   epoll_data_t data;	/* User data variable */
// } __EPOLL_PACKED;

namespace
{
enum ChannelStatus
{
    kNew = -1,      /* 未被添加到poller中 */
    kAdded = 1,     /* 已被添加到poller中，且有感兴趣事件 */
    kDeleted = 2    /* 已被添加到poller中，且没有感兴趣事件 */
};
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannls) const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    Channel* channel(nullptr);
    for(int i = 0; i < numEvents; i++)
    {
        channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannls->push_back(channel);
    }
}

void EpollPoller::update(int op, Channel *channel)
{
    epoll_event event;
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    LOG_DEBUG << "epoll_ctl op = " << operationToString(op)
    << " fd = " << fd << " event = { " << channel->eventToString() << " }";
    if (::epoll_ctl(epollFd_, op, fd, &event) < 0)
    {
        if (op == EPOLL_CTL_DEL)
        {
            LOG_ERROR << "epoll_ctl op =" << operationToString(op) << " fd =" << fd;
        }
        else
        {
            LOG_FATAL << "epoll_ctl op =" << operationToString(op) << " fd =" << fd;
        }
    }

}

EpollPoller::EpollPoller(EventLoop *loop):
    Poller(loop),
    epollFd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if(epollFd_ < 0)
    {
        LOG_FATAL << "EPollPoller::EPollPoller";
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollFd_);
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    this->assertInLoopThread();
    int numEvents = ::epoll_wait(epollFd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
    Timestamp now = Timestamp::now();
    
    if(numEvents > 0)
    {
        LOG_DEBUG << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size()*2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG << "no event happend";
    }
    else
    {
        LOG_ERROR << "EPollPoller::poll()";
    }
    
    return now;
}

void EpollPoller::updateChannel(Channel *channel)
{
    this->assertInLoopThread();
    const int index = channel->index();
    int fd = channel->fd();
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew) //kNew
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else              // kDeleted
        {
            // 移除channel时会保留其在channels中的键值对，故仍应该能被查询到
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // 如果没有感兴趣事件，则移除
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }

}

void EpollPoller::removeChannel(Channel *channel)
{
    this->assertInLoopThread();
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    const int index = channel->index();
    assert(channel->isNoneEvent());
    assert(index == kAdded || index == kDeleted);

    channels_.erase(fd);
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

const char *EpollPoller::operationToString(int op)
{
  switch (op)
  {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      return "Unknown Operation";
  }
}