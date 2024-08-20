#include "CurrentThread.h"
#include "Timestamp.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

__thread int CurrentThread::t_cachedTid = 0;
__thread const char* CurrentThread::t_threadName = nullptr;

void CurrentThread::cacheTid()
{
    if(t_cachedTid == 0)
    {
        t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
    }
}
    
bool CurrentThread::isMainThread()
{
    return ::getpid() == tid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
    struct timespec ts = { 0, 0 };
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    ::nanosleep(&ts, NULL);
}

std::string CurrentThread::stackTrace(bool demangle)
{
    std::string stack;
    const int max_frames = 200;
    void* frame[max_frames]; // 用于记录栈帧的地址
    int nptrs = ::backtrace(frame, max_frames); // 返回栈帧数量
    char** symbol = ::backtrace_symbols(frame, nptrs); // 返回地址的函数名和偏移

    if(symbol)
    {
        size_t len = 256;
        char* buf = demangle? static_cast<char*>(::malloc(len)) : nullptr;
        for(int i = 0; i < nptrs; i++) // 0th 即当前执行函数，可跳过
        {
            if (demangle)
            {
                char* mangled_name = nullptr;
                char* offset_begin = nullptr;
                char* offset_end = nullptr;
                // 找到括号和加号的位置，以提取函数名
                for (char* p = symbol[i]; *p; ++p) {
                    if (*p == '(') {
                        mangled_name = p;
                    } else if (*p == '+') {
                        offset_begin = p;
                    } else if (*p == ')') {
                        offset_end = p;
                        break;
                    }
                }

                if(mangled_name && offset_begin && offset_end)
                {
                    *mangled_name ++ = '\0';
                    *offset_begin = '\0';
                    *offset_end = '\0';
                    int status;
                    char* real_name = abi::__cxa_demangle(mangled_name, buf, &len, &status);
                    *offset_begin = '+';
                    if(status == 0)
                    {
                        stack.append(symbol[i]);
                        stack.append(" : ");
                        stack.append(offset_begin);
                        stack.push_back('\n');
                    }
                }
            }
            else
            {
                stack.append(symbol[i]);
                stack.push_back('\n');
            }
        }
        free(buf);
        free(symbol);
    }
    return stack;
}