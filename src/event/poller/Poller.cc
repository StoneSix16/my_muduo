#include "event/poller/Poller.h"
#include "event/EventLoop.h"

Poller::Poller(EventLoop* loop):
    loop_(loop),
    channels_()
{
}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const
{
  Utils::assertInLoopThread(loop_);
  auto it = channels_.find(channel->fd());
  return (it != channels_.end() && it->second == channel);
}