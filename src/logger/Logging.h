#pragma once

#include "base/Timestamp.h"
#include "logger/LogStream.h"
#include <functional>
#include <string.h>

namespace Utils
{
    const char* strerror_tl(int savedErrno);
}

/// @brief 读取文件名
class SourceFile
{
public:
    SourceFile(const char* filename):
        data_(filename)
    {
        const char* slash = strrchr(filename, '/');
        if(slash)
        {
            data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
    }
    const char* data_;
    int size_;
};


/// @brief 日志：构造时将内容保存到缓冲区。析构时通过回调函数输出缓冲区内容。
class Logger
{
public:
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();

    LogStream& stream() {return impl_.stream_; }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    using OutputFunc = std::function<void(const char* msg, int len)>;
    using FlushFunc = std::function<void()>;
    static void setOutput(OutputFunc func);
    static void setFlush(FlushFunc func);

private:
    /// @brief 内部类，用于实现日志的输出
    class Impl
    {
    public:
        using LogLevel = Logger::LogLevel;
        /// @brief 构造并输出日志信息
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
        /// @brief 输出时间
        void formatTime();
        /// @brief 输出当前位置
        void finish();

        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };

    Impl impl_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
    return g_logLevel;
}

// 获取errno信息
// const char* getErrnoMsg(int savedErrno);

#define LOG_DEBUG if (Logger::logLevel() <= Logger::DEBUG) \
  Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
#define LOG_INFO if (Logger::logLevel() <= Logger::INFO) \
  Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()