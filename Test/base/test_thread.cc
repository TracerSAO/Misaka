#include "../../base/Thread.h"

#include <unistd.h>

#include <cstdio>

void threadFunc()
{
    printf("threadFunc()\n");
}
void test1()
{
    Misaka::Thread thread(std::bind(&threadFunc), "firstTemp");
    thread.start();

    printf("main - thread\n");
    usleep(400 * 1000);
}

void test2_threadFunc()
{
    printf(">>> fork-son process\n");
    int res = fork();
    if (0 == res) {
        printf("@@@son-proc@@@ t_threadName: %s\n",
                Misaka::CurrentThread::threadName());
    }
    else {
        printf("@@@father-o_threa@@@ t_threadName:%s\n", 
                Misaka::CurrentThread::threadName());
    }
}
void test2_fork()
{
    Misaka::Thread thread(std::bind(&test2_threadFunc));
    thread.start();
    printf("@@@father-thread@@@ t_threadName:%s\n",
            Misaka::CurrentThread::threadName());
    usleep(100 * 1000);
}

int main()
{
    test2_fork();
}