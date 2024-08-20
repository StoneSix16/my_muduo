#pragma once

#include "base/Thread.h"
#include "logger/FixedBuffer.h"

#include <semaphore.h>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
/// @brief 异步日志，分为前端与后端。前端写入缓冲区，后端交换缓冲区内容并写入文件
class AsyncLogging
{
public:
    AsyncLogging(const std::string& basename,
                 off_t rollSize,
                 int flushInterval = 3
    );

    ~AsyncLogging()
    {
        if(running_)
        {
            stop();
        }
    }

    /// @brief 供前端调用，将数据写入前端缓冲区
    void append(const char* data, int len);

    void start()
    {
        running_ = true;
        thread_.start();
        // 启动线程，并确保线程开始执行回调函数后退出
        sem_wait(&sem_);
    }
    void stop()
    {
        running_ = false;
        cond_.notify_all();
        thread_.join();
    }
private:
    void threadFunc();

    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    bool running_;
    const off_t rollSize_;
    const int flushInterval_;
    const std::string basename_;

    Thread thread_;
    sem_t sem_;
    std::mutex mutex_;
    std::condition_variable cond_;
    BufferPtr currentBuffer_;   /* 前端buffer1 */
    BufferPtr nextBuffer_;      /* 前端buffer2 */
    BufferVector buffers_;
};


