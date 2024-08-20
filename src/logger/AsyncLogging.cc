#include "logger/AsyncLogging.h"
#include "logger/LogFile.h"

#include <assert.h>


AsyncLogging::AsyncLogging(const std::string& basename,
                 off_t rollSize,
                 int flushInterval):
    basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    running_(false),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "logging"),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
    sem_init(&sem_, false, 0);
}

void AsyncLogging::append(const char* data, int len)
{
    assert(running_);
    std::lock_guard<std::mutex> lock(mutex_);
    if(currentBuffer_->avail() >= len)
    {   
        currentBuffer_->append(data, len);
    }
    else
    {
        buffers_.push_back(std::move(currentBuffer_));
        if(nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            // 备用缓冲区也不够
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(data, len);
        // 唤醒后端线程
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc()
{
    // 写入磁盘的接口
    logFile output(basename_, rollSize_, 0);
    sem_post(&sem_);
    BufferPtr newBuffer1(new Buffer), newBuffer2(new Buffer);

    newBuffer1->bzero();
    newBuffer2->bzero();

    BufferVector bufferToWrite;
    bufferToWrite.reserve(16);
    while(running_)
    {
        
        // 获取前端缓冲区的内容
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 如果缓冲区为空，则等待一段时间
            if(buffers_.empty())
            {
                cond_.wait_for(lock, std::chrono::seconds(3));
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffers_.swap(bufferToWrite);
            if(!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        for(const auto& buffer: bufferToWrite)
        {
            output.append(buffer->data(), buffer->length());
        }

        // 只保留两个缓冲区
        if (bufferToWrite.size() > 2)
        {
            bufferToWrite.resize(2);
        }

        // 归还newBuffer1缓冲区
        if (!newBuffer1)
        {
            assert(!bufferToWrite.empty());
            newBuffer1 = std::move(bufferToWrite.back());
            bufferToWrite.pop_back();
            newBuffer1->reset();
        }

        // 归还newBuffer2缓冲区
        if (!newBuffer2)
        {
            assert(!bufferToWrite.empty());
            newBuffer2 = std::move(bufferToWrite.back());
            bufferToWrite.pop_back();
            newBuffer2->reset();
        }
        bufferToWrite.clear();
        output.flush();
    }
    output.flush();
}