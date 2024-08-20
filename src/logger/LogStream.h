#pragma once

#include "FixedBuffer.h"

#include <string.h>


/// @brief 使用4KB缓冲区封装的流
class LogStream : noncopyable
{
    using self = LogStream;
public:
    using Buffer = FixedBuffer<kSmallBuffer>;

    self& operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);
    self& operator<<(const void*);
    self& operator<<(float);
    self& operator<<(double);

    self& operator<<(char v);
    self& operator<<(const char* str);
    self& operator<<(const unsigned char* str);
    self& operator<<(const std::string& v);
    self& operator<<(const Buffer& v);

    /// @brief 添加字符串到缓冲区
    void append(const char* data, int len) { buffer_.append(data, len); }

    /// @brief 返回buffer内容（不可修改）
    const Buffer& buffer() const { return buffer_; }

    /// @brief 重置buffer当前指针
    void resetBuffer() { buffer_.reset(); }

private:

    template<typename T>
    void formatInteger(T);

    Buffer buffer_;

    static const int kMaxNumericSize = 48;
};

/// @brief 格式化类
class Fmt
{
public:
    template<typename T>
    Fmt(const char* fmt, T val)
    {
        length_ = snprintf(buf_, sizeof(buf_), fmt, val);
    }

    const char* data() const { return buf_; }
    int length() const { return length_; }

private:
    char buf_[32];
    int length_;
};

/// @brief 输出指定长度的字符串
class MonoFmt
{
public:
    MonoFmt(const char* data, int len);

    const char* data() const { return buf_; }
    int length() const { return length_; }

private:
    char buf_[32];
    int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
{
  s.append(fmt.data(), fmt.length());
  return s;
}

inline LogStream& operator<<(LogStream& s, const MonoFmt& fmt)
{
  s.append(fmt.data(), fmt.length());
  return s;
}