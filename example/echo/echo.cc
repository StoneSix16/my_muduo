#include "base/noncopyable.h"
#include "event/EventLoop.h"
#include "logger/Logging.h"
#include "net/TcpServer.h"
#include "net/TcpConnection.h"
#include "net/Buffer.h"

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& listenAddr, const std::string name):
        loop_(loop),
        tcpServer_(loop, name, listenAddr)
    {
        using namespace std::placeholders;
        tcpServer_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, _1));
        
        tcpServer_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, _1, _2, _3));

        tcpServer_.setThreadNum(1);
    }
    ~EchoServer() = default;

    void start()
    {
        tcpServer_.start();
    }
private:
        // 连接建立或断开的回调函数
    void onConnection(const TcpConnectionPtr &conn)   
    {
        if (conn->connected())
        {
            LOG_INFO << "Connection UP : " << conn->peerAddress().toIpPort().c_str();
        }
        else
        {
            LOG_INFO << "Connection DOWN : " << conn->peerAddress().toIpPort().c_str();
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
                 << "data received at " << time.toFormattedString();
        conn->send(msg);
        // conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }

    EventLoop* loop_;

    TcpServer tcpServer_;
};

int main()
{
    EventLoop loop;
    EchoServer server(&loop, InetAddress(4534), "echo");
    server.start();
    loop.loop();
    
    return 0;
}