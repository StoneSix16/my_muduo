#include "net/Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include "Buffer.h"

const char Buffer::kCRLF[] = "\r\n";

ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char extraBuf[65536] = {0};

    // 用一个额外缓冲区用于暂存socket的输出
    struct iovec vec[2];
    size_t writable = writableBytes();
    vec[0].iov_base = beginWrite(); 
    vec[0].iov_len = writable;
    vec[1].iov_base = extraBuf; 
    vec[1].iov_len = sizeof(extraBuf);

    // 如果buffer_可写空间>=64kb，则不使用额外缓冲区
    int vecNum = writable < sizeof(extraBuf)? 2 : 1;
    ssize_t n = readv(fd, vec, vecNum);

    if(n < 0)
    {
        *savedErrno = errno;
    }
    else if(n <= writable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extraBuf, n-writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int *savedErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *savedErrno = errno;
    }
    else if(n >= 0)
    {
        retrieve(n);
    }
    return n;
}