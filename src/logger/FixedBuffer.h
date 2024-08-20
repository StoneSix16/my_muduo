#pragma once

#include "base/noncopyable.h"

#include <memory>
#include <string.h>

const int kSmallBuffer = 4*1024;
const int kLargeBuffer = 4*1024*1024;


/// @brief 指定大小的缓冲区，一般为4KB/4MB
/// @tparam SIZE 缓冲区大小
template<int SIZE>
class FixedBuffer: noncopyable
{
public:
    FixedBuffer():
        cur_(data_)
    {
    }
    ~FixedBuffer()
    {
    }

    /// @brief 如果空间足够，向缓冲区内添加内容
    void append(const char* /*restrict*/ buf, size_t len)
    {
        // FIXME: append partially
        if (static_cast<size_t>(avail()) > len)
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }

    
    /// @brief 返回当前指针
    char* current() const { return cur_; }
    /// @brief 可用空间
    int avail() const { return static_cast<int>(end() - cur_); }
    /// @brief 移动当前指针
    void add(size_t len) { cur_ += len; }

    
    /// @brief 重置指针
    void reset() { cur_ = data_; }
    /// @brief 重置缓冲区为0
    void bzero() { memset(data_, 0, sizeof(data_)); }

    std::string toString() const { return std::string(data_, length()); }

private:
    const char* end() const { return data_ + sizeof(data_); }

    char data_[SIZE];
    char* cur_;
};
