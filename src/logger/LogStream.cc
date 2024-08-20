#include "logger/LogStream.h"

#include <algorithm>

namespace Utils
{
    const char digits[] = "9876543210123456789";
    const char* zero = digits + 9;
    const char digitsHex[] = "0123456789ABCDEF";

    template<typename T>
    size_t convert(char buf[], T value)
    {
        T x = value; 
        char* p = buf;
        do
        {
            int lsd = static_cast<int>(x%10);
            x /= 10;
            *p++ = zero[lsd];
        } while(x != 0);

        if(value < 0)
        {
            *p++ = '-';
        }

        *p = '\0';
        std::reverse(buf, p);
        return p-buf;
    }

    size_t convertHex(char buf[], uintptr_t value)
    {
        uintptr_t i = value;
        char* p = buf;

        do
        {
            int lsd = static_cast<int>(i % 16);
            i /= 16;
            *p++ = digitsHex[lsd];
        } while (i != 0);

        *p = '\0';
        std::reverse(buf, p);

        return p - buf;
    }

}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

LogStream& LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(float v) 
{
    *this << static_cast<double>(v);
    return *this;
}

LogStream& LogStream::operator<<(double v) 
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char buf[32];
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v); 
        buffer_.add(len);
    }
    return *this;
}

LogStream& LogStream::operator<<(char c)
{
    buffer_.append(&c, 1);
    return *this;
}

LogStream& LogStream::operator<<(const void* data) 
{
    *this << static_cast<const char*>(data); 
    return *this;
}

LogStream& LogStream::operator<<(const char* str)
{
    if (str)
    {
        buffer_.append(str, strlen(str));
    }
    else 
    {
        buffer_.append("(null)", 6);
    }
    return *this;
}

LogStream& LogStream::operator<<(const unsigned char* str)
{
    return operator<<(reinterpret_cast<const char*>(str));
}

LogStream& LogStream::operator<<(const std::string& str)
{
    buffer_.append(str.c_str(), str.size());
    return *this;
}

LogStream& LogStream::operator<<(const Buffer& buf)
{
    *this << buf.toString();
    return *this;
}

template<typename T>
void LogStream::formatInteger(T value)
{
    if(buffer_.avail() >= kMaxNumericSize)
    {
        size_t len = Utils::convert(buffer_.current(), value);
        buffer_.add(len);
    }
}

template Fmt::Fmt(const char* fmt, char);
template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);
template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);

MonoFmt::MonoFmt(const char *data, int len):
    length_(len)
{
    snprintf(buf_, sizeof(buf_), "%s", data);
}