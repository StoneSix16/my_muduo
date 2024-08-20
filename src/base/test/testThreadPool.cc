#include "base/ThreadPool.h"
#include "base/CurrentThread.h"
#include <stdio.h>
#include <unistd.h>
#include <functional>

int count = 0;

void showInfo()
{
    fprintf(stdout,"execute tid = %d\n", CurrentThread::tid());
}

void test1()
{
    ThreadPool pool;

    pool.setMaxQueueSize(6000);
    pool.start(4);    
    for (int i = 0; i < 5000; i++) 
    {
        pool.add(showInfo);
    }
    pool.add([]{sleep(3);});
    // pool.stop();
}

void initFunc()
{
    printf("Create thread %d\n", ++count);
}

int main()
{
    test1();
    
    return 0;
}