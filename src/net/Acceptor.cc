#include "Acceptor.h"
#include "logger/Logging.h"
#include "net/InetAddress.h"
#include "event/EventLoop.h"

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort):
    loop_(loop),
    acceptSocket_(Socket::createNoblockingOrDie(listenAddr.family())),
    acceptChannel_(loop, acceptSocket_.fd()),
    listening_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reusePort);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    Utils::assertInLoopThread(loop_);
    listening_ = true;
    acceptChannel_.enableReading();
    acceptSocket_.listen();
}

void Acceptor::handleRead()
{
    Utils::assertInLoopThread(loop_);
    InetAddress peerAddr;
    int connFd = acceptSocket_.accept(&peerAddr);
    if(connFd >= 0)
    {
        if(newConnectionCallback_)
        {
            newConnectionCallback_(connFd, peerAddr);
        }
        else
        {
            ::close(connFd);
        }
    }
    else
    {
        LOG_ERROR << "in Acceptor::handleRead";
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        /// 进程打开的文件描述符达到上限, 关闭预占用的idleFd, 通过accept获取连接的fd并关闭连接.  
        if (errno == EMFILE)
        {
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
