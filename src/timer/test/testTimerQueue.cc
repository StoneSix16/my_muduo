#include "base/CurrentThread.h"
#include "base/Timestamp.h"
#include "base/ThreadPool.h"
#include "logger/AsyncLogging.h"
#include "logger/Logging.h"
#include "event/EventLoop.h"
#include "timer/TimerQueue.h"

#include <semaphore.h>
#include <random>
#include <assert.h>

static const off_t rollSize = 1024*1024;
AsyncLogging* g_logger;

void asyncOutput(const char* msg, int len)
{
    assert(g_logger);
    if(g_logger)
    {
        g_logger->append(msg, len);
    }
}

void createLoop(EventLoop** loopPtr, sem_t* sem)
{
    EventLoop* loop = new EventLoop();
    *loopPtr = loop;
    LOG_INFO << "Loop Thread " << CurrentThread::tid() << " is created";
    sem_post(sem);
    loop->loop();

    delete loop;
}

void logInfo(Timestamp idealTime, pid_t threadId)
{
    std::string time = Timestamp::now().toFormattedString(); 
    LOG_INFO << "logInfo is called in Thread " << threadId << " at " << time
             << ", which should be called at " << idealTime.toFormattedString();
}

void threadCallback(EventLoop* loop)
{
    Timestamp time = Timestamp::now();
    pid_t threadId = CurrentThread::tid();
    LOG_INFO << "Log Thread " << threadId << " is created";
    std::function<void()> init = [time, threadId]() 
                                 {
                                    LOG_INFO << "runInLoop() is tested in Thread " << threadId << " at " << time.toFormattedString();
                                 };
    loop->queueInLoop(init);
    for(int iteration = 0; iteration < 1; iteration++)
    {
        double delay = 0.5;
        std::function<void()> callback = std::bind(logInfo, Timestamp::now(), threadId);
        // loop->runAfter(delay, callback);
        // loop->runAt(Timestamp::now(), callback);
        loop->runEvery(delay, callback);
    }
}

int main(int argc, char* argv[])
{
    EventLoop* loop;
    sem_t loopSem; 
    sem_init(&loopSem, false, 0);
    Thread* loopThread = new Thread(std::bind(createLoop, &loop, &loopSem));
    g_logger = new AsyncLogging(argv[0], rollSize);
    // ThreadPool* threadPool = new ThreadPool();
    std::vector<Thread*> threads;
    // Logger::setLogLevel(Logger::DEBUG);
    Logger::setOutput(asyncOutput);
    g_logger->start();

    LOG_INFO << "Main Thread " << CurrentThread::tid() << " is created";
    loopThread->start();
    sem_wait(&loopSem);
    for(int i = 0; i < 4; i++)
    {
        threads.push_back(new Thread(std::bind(threadCallback, loop)));
        threads.back()->start();
        LOG_INFO << threads.back()->started();
    }

    sleep(5);
    
    for(Thread* thread: threads)
    {
        thread->join();
        delete thread;
    }
    loop->quit();
    loopThread->join();
    delete loopThread;

    g_logger->stop();

}