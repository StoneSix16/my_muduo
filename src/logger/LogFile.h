#pragma once

#include "base/noncopyable.h"
#include "logger/Logging.h"

#include <string>
#include <mutex>
#include <stdint.h>

namespace Utils
{
    /// @brief 辅助类，用于打开并写入文件
    class FileUtil : noncopyable
    {
    public:
        /// @brief 打开文件并设置缓冲区
        /// @param fileName 文件名
        explicit FileUtil(const std::string& fileName);

        /// @brief 关闭文件
        ~FileUtil();

        /// @brief 向文件缓冲区添加内容
        void append(const char* data, size_t len);

        /// @brief 刷新缓冲区
        void flush();

        off_t writtenBytes() const { return writtenBytes_; }

    private:    
        size_t write(const char* data, size_t len);

        FILE* fp_;
        char buffer_[64 * 1024]; // fp_的缓冲区
        off_t writtenBytes_; // off_t用于指示文件的偏移量
    };

}

class logFile
{
public:
    logFile(const std::string& name,
        off_t rollSize,
        int flushInterval = 3,
        int checkEveryN = 1024);
    ~logFile() = default;

    /// @brief 向文件缓冲区添加内容
    void append(const char* data, int len);
    /// @brief 刷新文件缓冲区
    void flush();
    /// @brief 根据当前时间注册新的日志文件
    bool rollFile();

private:
    void append_unclocked(const char* logline, int len);
    
    /// 根据时间戳获取日志文件名
    static std::string getLogFileName(const std::string& basename, time_t* now);

    const std::string basename_;
    const off_t rollSize_;
    const int flushInterval_;
    const int checkEveryN_;

    int count_;

    std::mutex mutex_;
    std::unique_ptr<Utils::FileUtil> file_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;

    const static int kRollPerSeconds_ = 60*60*24;
};