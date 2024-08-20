#include "logger/Logging.h"
#include "net/Acceptor.h"
#include "net/TcpConnection.h"
#include "net/TcpServer.h"
#include "event/EventLoop.h"
#include "event/EventLoopThreadPool.h"

#include <functional>
#include <assert.h>
#include <string>

TcpServer::TcpServer(EventLoop *loop, const std::string &name, const InetAddress &listenAddr, Option option):
    loop_(loop),
    name_(name),
    ipPort_(listenAddr.toIpPort()),
    started_(false),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, name)),
    nextConnId(1)
{
    using namespace std::placeholders;
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, _1, _2)
    );
}

TcpServer::~TcpServer()
{
    Utils::assertInLoopThread(loop_);
    for(auto& item: connections_)
    {
        item.second->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, item.second)
        );
    }
}

void TcpServer::setThreadNum(int threadNum)
{
    threadPool_->setNumThread(threadNum);
}

void TcpServer::start()
{
    int expect = 1;
    if(started_.compare_exchange_weak(expect, 1) == 0)
    {
        threadPool_->start();
        assert(!acceptor_->listening());
        loop_->runInLoop(
            std::bind(&Acceptor::listen, acceptor_.get())
        );
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    Utils::assertInLoopThread(loop_);   
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64];
    std::string connName(name_ + ipPort_ + std::to_string(nextConnId));
    nextConnId++;

    LOG_INFO << "TcpServer::newConnection [" << name_
        << "] - new connection [" << connName
        << "] from " << peerAddr.toIpPort();
    InetAddress localAddr(Utils::getLocalAddr(sockfd));

    TcpConnectionPtr conn(std::make_shared<TcpConnection>(ioLoop,
                                                            connName,
                                                            sockfd,
                                                            localAddr,
                                                            peerAddr
                                                            ));

    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        [this](const TcpConnectionPtr& conn)
        {
            loop_->runInLoop(
                std::bind(&TcpServer::removeConnection, this, conn)
            );
        });
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    Utils::assertInLoopThread(loop_);
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
        << "] - connection " << conn->name();
    connections_.erase(conn->name());

    conn->getLoop()->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}
