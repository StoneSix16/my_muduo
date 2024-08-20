#include "event/poller/Poller.h"
#include "event/poller/EpollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    return new EpollPoller(loop);
}