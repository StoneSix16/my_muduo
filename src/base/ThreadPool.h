#pragma once

#include "base/noncopyable.h"
#include "base/Thread.h"

#include <vector>
#include <deque>
#include <condition_variable>
#include <mutex>

class ThreadPool: noncopyable
{
public:
    using Task = std::function<void()>; 

    explicit ThreadPool(const std::string& name = std::string("ThreadPool"));
    ~ThreadPool();

    // 需要在start之前配置
    void setMaxQueueSize(int maxSize) {maxQueueSize_ = maxSize; }
    void setThreadInitCallback(const Task& task) { threadInitCallback_ = task; }

    
    /// @brief 初始化线程，线程函数为runInThread
    /// @param numThreads 
    void start(int numThreads);
    
    /// @brief 结束所有线程，将所有线程唤醒并join
    void stop();

    /// @brief 添加一个task
    /// @param f 
    void add(Task f);

    const std::string& name() const {return name_;}

    size_t queueSize() const;
private:
    /// @brief  线程内部的回调函数，该函数持续尝试获取一个task并执行
    void runInThread();

    mutable std::mutex mutex_;  /* task队列互斥量 */
    std::condition_variable notEmpty_, notFull_;    /* 判断队列是否空/满 */
    std::string name_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<Thread>> threads_;
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};
