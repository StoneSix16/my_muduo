#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>
#include <assert.h>

std::atomic<int> Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name):
    started_(false),
    joined_(false),
    tid_(0),
    func_(std::move(func)),
    name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    // 调用join后，caller阻塞知道对应线程的join返回
    // 调用detach后，caller不再关注对应线程，线程在后台运行并由操作系统掌控
    if(started_ && !joined_){
        thread_->detach();
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    // 使用lambda创建含参数线程，需要在新建线程内获取tid并传出给tid_
    thread_ = std::make_unique<std::thread>([this, &sem](){
        this->tid_ = CurrentThread::tid();
        // 为了防止线程获取到tid前被别的线程访问。
        sem_post(&sem);
        try
        {
            func_();
            CurrentThread::t_threadName = "finished";
        }
        catch(const std::exception& e)
        {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", e.what());
            fprintf(stderr, "stack trace: %s\n", CurrentThread::stackTrace(/*demangle*/true).c_str());
            abort();
        }
        catch (...)
        {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw; // rethrow
        }
    });

    sem_wait(&sem);
}

void Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}
