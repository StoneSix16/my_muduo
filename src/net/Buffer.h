#pragma once 
#include <vector>
#include <algorithm>
#include <string>
#include <stdint.h>
#include <assert.h>
#include <string.h>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer
{
public:
    enum IntSize
    {
        INT8,
        INT16,
        INT32,
        INT64,
    };

    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    
    explicit Buffer(size_t initialSize = kInitialSize):
        buffer_(kCheapPrepend + initialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
    {
    }
    
    void swap(Buffer& rhs)
    {
        std::swap(buffer_, rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    size_t readableBytes() const {return writerIndex_ - readerIndex_; }
    size_t writableBytes() const {return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const {return readerIndex_; }
    /// @brief 获取readerindex位置地址
    const char* peek() const {return begin() + readerIndex_;}

    const char* findCRLF(const char* start = nullptr) const
    {
        if(start == nullptr) start = beginRead();
        assert(start >= beginRead());
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite()? nullptr : crlf;
    }
    const char* findEOL(const char* start = nullptr) const
    {
        if(start == nullptr) start = beginRead();
        assert(start >= beginRead());
        assert(start <= beginWrite());
        const void* eol = memchr(beginRead(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    /// @brief 复位操作，用于读取缓冲区
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if(len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveUtil(const char* end)
    {
        assert(end <= beginWrite());
        assert(end > beginRead());
        retrieve(end - beginRead());
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string result(beginRead(), len);
        retrieve(len);
        return result;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }


    /// @brief 写操作，用于写入缓冲区。缓冲区空间不足时会利用vector特性扩容
    void append(const char* /*restrict*/ data, size_t len)
    {
        ensureWritable(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    void append(const void* /*restrict*/ data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }


    void append(const std::string& str)
    {
        append(str.data(), str.size());
    }

    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

    void ensureWritable(size_t len)
    {
        if(len > writableBytes())
        {
            makeSpace(len);
        }
        assert(len <= writableBytes());
    }

    void prepend(const void* /*restrict*/ data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }

    void shrink(size_t reserve)
    {
        // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
        Buffer other;
        other.ensureWritable(readableBytes()+reserve);
        other.append(beginRead(), readableBytes());
        swap(other);
    }

    size_t internalCapacity() const
    {
        return buffer_.capacity();
    }

    char* beginRead() {return begin() + readerIndex_; }
    char* beginWrite() {return begin() + writerIndex_; }
    const char* beginRead() const {return begin() + readerIndex_; }
    const char* beginWrite() const {return begin() + writerIndex_; }

    /// 基于int类型的数据读取和写入
    template<typename T>
    void peekInt(T* value) const
    {
        assert(readableBytes() >= sizeof(T));
        ::memcpy(value, beginRead(), sizeof(T));
    }

    template<typename T>
    void readInt(T* value)
    {
        peekInt(value);
        retrieve(sizeof(T));
    }

    template<typename T>
    void appendInt(T value)
    {
        append(&value, sizeof(T));
    }

    template<typename T>
    void prependInt(T value)
    {
        prepend(&value, sizeof(T));
    }


    /// @brief 直接从缓冲区读取数据，通过readv(2)实现
    ssize_t readFd(int fd, int* savedErrno);
    /// @brief 将缓冲区数据输出到fd中, 通过write实现
    ssize_t writeFd(int fd, int* savedErrno);
private:
    const char* begin() const {return &(*buffer_.begin()); }
    char* begin() {return &(*buffer_.begin()); }

    /// @brief 扩容函数。如果prependable和writable空间足够，则通过前移readable部分以腾出空间
    /// @param len 
    void makeSpace(size_t len)
    {
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);
        }
        else
        {
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(beginRead(), beginWrite(), begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_+readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];
};

template void Buffer::peekInt<int8_t>(int8_t*) const;
template void Buffer::peekInt<int16_t>(int16_t*) const;
template void Buffer::peekInt<int32_t>(int32_t*) const;
template void Buffer::peekInt<int64_t>(int64_t*) const;

template void Buffer::readInt<int8_t>(int8_t*);
template void Buffer::readInt<int16_t>(int16_t*);
template void Buffer::readInt<int32_t>(int32_t*);
template void Buffer::readInt<int64_t>(int64_t*);

template void Buffer::appendInt<int8_t>(int8_t);
template void Buffer::appendInt<int16_t>(int16_t);
template void Buffer::appendInt<int32_t>(int32_t);
template void Buffer::appendInt<int64_t>(int64_t);

template void Buffer::prependInt<int8_t>(int8_t);
template void Buffer::prependInt<int16_t>(int16_t);
template void Buffer::prependInt<int32_t>(int32_t);
template void Buffer::prependInt<int64_t>(int64_t);

