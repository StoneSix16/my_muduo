#include "logger/Logging.h"
#include "event/Channel.h"
#include "event/EventLoop.h"

#include <sstream>
#include <assert.h>
#include <poll.h>

Channel::Channel(EventLoop *eventLoop, int fd):
    loop_(eventLoop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1),
    tied_(false),
    eventHandling_(false),
    addedToLoop_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
    assert(!addedToLoop_);
    if (loop_->isInLoopThread())
    {
        assert(!loop_->hasChannel(this));
    }
}

void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::tie(const std::shared_ptr<void> &ptr)
{
    tie_ = ptr;
    tied_ = true;
}

void Channel::remove()
{
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

std::string Channel::eventToString()
{
    std::ostringstream oss;
    oss << fd_ << ": ";
    if (events_ & POLLIN)
    oss << "IN ";
    if (events_ & POLLPRI)
    oss << "PRI ";
    if (events_ & POLLOUT)
    oss << "OUT ";
    if (events_ & POLLHUP)
    oss << "HUP ";
    if (events_ & POLLRDHUP)
    oss << "RDHUP ";
    if (events_ & POLLERR)
    oss << "ERR ";
    if (events_ & POLLNVAL)
    oss << "NVAL ";

    return oss.str();
}
void Channel::update()
{
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    /*
        poll事件
        可读：POLLIN POLLRDNORM POLLRDBAND POLLPRI
        可写：POLLOUT POLLWRNORM POLLWRBAND
        错误：POLLERR POLLHUP POLLNVAL
    */
    eventHandling_ = true;
    LOG_WARN << eventToString();
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & POLLNVAL)
    {
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}
