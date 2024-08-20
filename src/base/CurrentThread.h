#pragma once

#include <string>
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    // __thread 关键字表示这是一个线程局部变量
    extern __thread int t_cachedTid;
    extern __thread const char* t_threadName;

    void cacheTid();

    /// @brief 获取缓存的线程tid，如果未缓存，通过系统调用获取并缓存新的tid
    /// @return tid
    inline int tid()
    {
        // gcc内建函数。第一个参数为布尔表达式，第二个参数为表达式为真的可能性。
        // 用于优化分支预测
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }

    inline const char* name()
    {
        return t_threadName;
    }

    bool isMainThread();

    void sleepUsec(int64_t usec);  // for testing

    /// @brief 打印堆栈信息
    /// @param demangle 是否将被编译器混淆的函数解码
    /// @return stacktrance
    std::string stackTrace(bool demangle);
}