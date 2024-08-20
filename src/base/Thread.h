#pragma once

#include "base/noncopyable.h"
#include <thread>
#include <functional>
#include <atomic>

class Thread: noncopyable
{
public:
    using ThreadFunc = std::function<void()>;
    
    // 传递一个函数对象以构造线程
    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }
private:

    /// @brief 更新numCreated_并设置索引，需要注意此时线程tid是非法的（尚未构造std::thread）。
    void setDefaultName();

    std::unique_ptr<std::thread> thread_;
    bool started_;
    bool joined_;
    pid_t tid_; // 线程tid（所有进程中唯一）
    ThreadFunc func_; // 线程会调用的回调函数
    std::string name_;

    static std::atomic<int> numCreated_;
};