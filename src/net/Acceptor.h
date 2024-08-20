#pragma once

#include "base/Callback.h"
#include "net/Socket.h"
#include "event/Channel.h"

class InetAddress;
class EventLoop;

class Acceptor: noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& address)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    void listen();

    bool listening() const {return listening_;}
private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;   /* 监听socket */
    Channel acceptChannel_;     
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    int idleFd_;
};
