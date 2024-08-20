#pragma once

#include "base/noncopyable.h"

struct sockaddr_in;
class InetAddress;

namespace Utils
{
    sockaddr_in getLocalAddr(int sockfd);
}

class Socket: noncopyable
{
public:
    explicit Socket(int fd):
        sockfd_(fd)
    {}

    ~Socket();

    /// @brief 获取sockfd
    int fd() const {return sockfd_; }
    /// @brief 获取TCP连接信息
    /// @return 是否获取成功
    bool getTcpInfoString (char* buf, int len) const;

    /// @brief 绑定socket。若地址被使用则中止
    void bindAddress(const InetAddress& localaddr);

    /// @brief 使sock可以接受连接。若地址被使用则中止
    void listen();
    
    /// @brief 接受连接
    /// @param peeraddr 如果成功，则peeraddr被赋值
    /// @return 如果成功，返回接受的socket的描述符（一个非负数），否则返回-1
    int accept(InetAddress* peeraddr);

    // 设置半关闭
    void shutdownWrite();

    // 设置Nagel算法 
    void setTcpNoDelay(bool on);  

    // 设置地址复用
    void setReuseAddr(bool on);     

    // 设置端口复用
    void setReusePort(bool on);     

    // 设置长连接
    void setKeepAlive(bool on);     

    static int createNoblockingOrDie(int domain);

private:
    const int sockfd_;
};