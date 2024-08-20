#include "ThreadPool.h"
#include "CurrentThread.h"
#include <assert.h>

ThreadPool::ThreadPool(const std::string& name):
    mutex_(),
    notEmpty_(),
    notFull_(),
    name_(name),
    running_(false),
    maxQueueSize_(0)
{
}

ThreadPool::~ThreadPool()
{
    if(running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);
    for(int i = 0; i < numThreads; ++i)
    {
        std::string name = name_ + std::to_string(i+1);
        // 生成runInThread的函数对象（类成员函数需要绑定类实例指针）
        Task task = std::bind(&ThreadPool::runInThread, this);
        threads_.emplace_back(new Thread(task, name));
        threads_[i]->start();
    }

    if(numThreads == 0 && threadInitCallback_)
    {
        threadInitCallback_();
    }
}

void ThreadPool::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        notFull_.notify_all();
        notEmpty_.notify_all();
    }
    for(const auto& t: threads_)
    {
        t->join();
    }
}

size_t ThreadPool::queueSize() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void ThreadPool::add(Task task)
{
    if(threads_.empty())
    {
        task();
    }
    else
    {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [this](){return !running_ || queue_.size() < maxQueueSize_ || maxQueueSize_ == 0;});
        if(running_ && queue_.size() < maxQueueSize_ && maxQueueSize_ != 0)
        {
            queue_.push_back(std::move(task));
            notEmpty_.notify_one();
        }
    }
}

void ThreadPool::runInThread()
{
    try
    {
        if (threadInitCallback_)
        {
            threadInitCallback_();
        }
        while(running_)
        {
            Task task = nullptr;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                notEmpty_.wait(lock, [this](){return !running_ || !queue_.empty();});
                if(!queue_.empty() && running_)
                {
                    task = queue_.front();
                    queue_.pop_front();
                    if(maxQueueSize_ > 0)
                    {
                        notFull_.notify_one();
                    }
                }
            }
            if(task) task();
        }
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
    
}