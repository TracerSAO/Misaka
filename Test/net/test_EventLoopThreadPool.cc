#include "../../net/EventLoopThreadPool.h"
#include "../../base/CurrentThread.h"
#include "../../net/EventLoop.h"
#include "../../net/TimerId.h"

#include <cstdio>

using namespace Misaka;
using namespace net;


void workFunc()
{
    printf("t_threadName: %s\tt_tidString: %s",
            CurrentThread::threadName(),
            CurrentThread::tidString());
    printf("\t\t[%p]\n", workFunc);
}

void eventthreadinitfunc(EventLoop* loop)
{
    loop->runEvery(2, std::bind(workFunc));
}

int main()
{
    EventLoop loop;
    EventLoopThreadPool eventPool(&loop, "EventLoopThreadPool");
    eventPool.setThreadNum(3);
    eventPool.start(std::bind(&eventthreadinitfunc, _1));

    loop.runAfter(6, std::bind(&EventLoop::quit, &loop));
    loop.loop();
}