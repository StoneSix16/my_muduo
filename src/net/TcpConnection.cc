#include "event/Channel.h"
#include "event/EventLoop.h"
#include "logger/Logging.h"
#include "net/TcpConnection.h"
#include "net/Socket.h"

#include <functional>
#include <assert.h>


TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr):
    loop_(loop),
    name_(name),
    state_(kConnecting),
    reading_(false),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop_, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));

    LOG_INFO << "TcpConnection::ctor[" << name_.c_str() << "] at fd =" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO << "TcpConnection::dtor[" << name_.c_str() << "] at fd=" << channel_->fd() << " state=" << static_cast<int>(state_);
}

std::string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof(buf));
    return buf;
}

void TcpConnection::send(const void *message, int len)
{
    if(state_.load() == kConnected)
    {
        loop_->runInLoop(
            std::bind(&TcpConnection::sendInLoop, this, message, len));
    }
}

void TcpConnection::send(const std::string &message)
{
    send(message.c_str(), message.length());
}

void TcpConnection::send(Buffer *message)
{
    send(message->peek(), message->readableBytes());
    message->retrieveAll();
}

void TcpConnection::shutdown()
{
    int expect = kConnected;
    if(state_.compare_exchange_weak(expect, kDisconnecting))
    {
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
    else
    {
        LOG_DEBUG << "fail at shuting down TcpConnection " << name_ << " with state" << stateToString(expect);
    }
}

void TcpConnection::forceClose()
{
    int expect = kConnected;
    if(state_.compare_exchange_weak(expect, kDisconnecting) || 
       state_.load() == kDisconnecting)
    {
        loop_->runInLoop(std::bind(&TcpConnection::forceCloseInLoop, this));
    }
    else
    {
        LOG_DEBUG << "fail at forcing close TcpConnection " << name_ << " with state" << stateToString(expect);
    }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
    int expect = kConnected;
    if(state_.compare_exchange_weak(expect, kDisconnecting) || 
       state_.load() == kDisconnecting)
    {
        loop_->runAfter(seconds, std::bind(&TcpConnection::forceCloseInLoop, this));
    }
    else
    {
        LOG_DEBUG << "fail at forcing close TcpConnection " << name_ << " with state" << stateToString(expect);
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::stopRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

const char* TcpConnection::stateToString(int state)
{
  switch (state)
  {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

void TcpConnection::connectEstablished()
{
    Utils::assertInLoopThread(loop_);
    assert(state_.load() == kConnecting);
    state_.store(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    Utils::assertInLoopThread(loop_);
    if (state_.load() == kConnected)
    {
        state_.store(kDisconnected);
        channel_->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除掉
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    Utils::assertInLoopThread(loop_);
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(socket_->fd(), &savedErrno);
    if(n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)
    {
        handleClose();
    }   
    else
    {
        errno = savedErrno;
        LOG_ERROR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    Utils::assertInLoopThread(loop_);
    assert(channel_->isWriting());
    int savedErrno = 0;
    ssize_t n = outputBuffer_.writeFd(socket_->fd(), &savedErrno);
    if(n > 0)
    {
        if(outputBuffer_.readableBytes() == 0)
        {
            channel_->disableWriting();
            if(writeCompleteCallback_)
            {
                writeCompleteCallback_(shared_from_this());
            }
            if(state_.load() == kDisconnecting)
            {
                shutdownInLoop();
            }
        }
    }
    else
    {
        LOG_ERROR << "TcpConnection::handleWrite";
    }
}

void TcpConnection::handleError()
{
    int err;
    socklen_t optlen = static_cast<socklen_t>(sizeof(err));

    if (::getsockopt(socket_->fd(), SOL_SOCKET, SO_ERROR, &err, &optlen) < 0)
    {
        err = errno;
    }
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << Utils::strerror_tl(err);
}

void TcpConnection::handleClose()
{
    Utils::assertInLoopThread(loop_);
    assert(state_.load() == kDisconnecting || 
           state_.load() == kConnected);
    state_.store(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guard = shared_from_this();
    connectionCallback_(guard);
    closeCallback_(guard);
}

void TcpConnection::sendInLoop(const void *message, size_t len)
{
    Utils::assertInLoopThread(loop_);
    ssize_t nWritten = 0;
    size_t remaining = len;
    bool toBuffer = false;
    if(state_.load() == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    
    if(outputBuffer_.readableBytes() == 0 && !channel_->isWriting())
    {
        nWritten = ::write(socket_->fd(), message, len);
        if(nWritten >= 0)
        {
            remaining = len - nWritten;
            if(writeCompleteCallback_ && remaining == 0)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nWritten = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    toBuffer = true;
                }
            }
        }
    }

    if(toBuffer)
    {
        size_t curLen = outputBuffer_.readableBytes();
        if(curLen < highWaterMark_ &&
           curLen + remaining >= highWaterMark_ &&
           highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), curLen+remaining));
        }
        outputBuffer_.append(message, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceCloseInLoop()
{
    Utils::assertInLoopThread(loop_);
    int state = state_.load();
    if(state == kConnected || state == kDisconnecting)
    {
        handleClose();
    }
}

void TcpConnection::startReadInLoop()
{
    Utils::assertInLoopThread(loop_);
    if(!reading_ && !channel_->isReading())
    {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopReadInLoop()
{
    Utils::assertInLoopThread(loop_);
    if(reading_ && channel_->isReading())
    {
        channel_->disableReading();
        reading_ = false;
    }
}
