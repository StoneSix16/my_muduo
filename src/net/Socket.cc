#include "logger/Logging.h"
#include "net/Socket.h"
#include "net/InetAddress.h"

#include <unistd.h>
// #include <sys/socket.h>
#include <netinet/tcp.h>
#include "Socket.h"

namespace Utils
{
    sockaddr* sockaddr_cast(struct sockaddr_in* addr)
    {
        return static_cast<struct sockaddr*>(static_cast<void*>(addr));
    }

    sockaddr_in getLocalAddr(int sockfd)
    {
        struct sockaddr_in localaddr;
        memset(&localaddr, 0, sizeof(localaddr));
        socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
        if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
        {
            LOG_ERROR << "sockets::getLocalAddr";
        }
        return localaddr;
    }

}

Socket::~Socket()
{
    if (::close(sockfd_) < 0)
    {
        LOG_ERROR << "Socket::~Socket";
    }
}

bool Socket::getTcpInfoString(char *buf, int len) const
{
    struct tcp_info tcpi;
    socklen_t tcpiLen = sizeof(tcpi);
    memset(&tcpi, 0, len);
    bool ok = ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, &tcpi, &tcpiLen) == 0;
    if (ok)
    {
        snprintf(buf, len, "unrecovered=%u "
                "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                "lost=%u retrans=%u rtt=%u rttvar=%u "
                "sshthresh=%u cwnd=%u total_retrans=%u",
                tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
                tcpi.tcpi_rto,          // Retransmit timeout in usec
                tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
                tcpi.tcpi_snd_mss,
                tcpi.tcpi_rcv_mss,
                tcpi.tcpi_lost,         // Lost packets
                tcpi.tcpi_retrans,      // Retransmitted packets out
                tcpi.tcpi_rtt,          // Smoothed round trip time in usec
                tcpi.tcpi_rttvar,       // Medium deviation
                tcpi.tcpi_snd_ssthresh,
                tcpi.tcpi_snd_cwnd,
                tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
    }
    return ok;
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    int ret = ::bind(sockfd_, localaddr.getSockAddr(), static_cast<socklen_t>(sizeof(sockaddr_in)));
    if (ret < 0)
    {
        LOG_FATAL << "Socket::bindAddress";
    }
}

void Socket::listen()
{
    int ret = ::listen(sockfd_, SOMAXCONN);
    if (ret < 0)
    {
        LOG_FATAL << "Socket::listen";
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
    int connfd = ::accept4(sockfd_, Utils::sockaddr_cast(&addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    else
    {
        int savedErrno = errno;
        LOG_ERROR << "Socket::accept";
        LOG_FATAL << "unexpected error of ::accept " << savedErrno;
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR << "Socket::shutdownWrite";
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_ERROR << "TCP_NODELAY failed.";
    }
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                &optval, static_cast<socklen_t>(sizeof optval));
                if (ret < 0 && on)
    {
        LOG_ERROR << "SO_REUSEADDR failed.";
    }
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                            &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_ERROR << "SO_REUSEPORT failed.";
    }
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
                &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_ERROR << "SO_KEEPALIVE failed.";
    }
}

int Socket::createNoblockingOrDie(int domain)
{
    int sockfd = ::socket(domain, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL << "sockets::createNonblockingOrDie";
    }
    return sockfd;
}
