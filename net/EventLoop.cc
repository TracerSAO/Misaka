#include "../base/Timestamp.h"
#include "EventLoop.h"
#include "Epoller.h"
#include "Channel.h"
#include "TimerId.h"
#include "TimerQueue.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/eventfd.h>

using namespace Misaka;
using namespace net;

namespace
{

    __thread EventLoop* t_loopInthisThread = 0; // 为什么要使用 0 ，而不是 nullptr ?

    int createEventfd()
    {
        int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (0 > eventfd)
        {
            assert(0 < eventfd); // 尚未构造 LOG class，暂使用 assert 断言来临时替换
        }
        return eventfd;
    }

    class IgnoreSigPipe
    {
    public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
        }
    };

    IgnoreSigPipe ignoreSigPipe;    // 利用全局对象，在 server 启动前，直接忽略 SIGPIPE 信号，避免进程突然退出
}

EventLoop::EventLoop():
    quit_(false),
    looping_(false),
    threadId_(pthread_self()),
    callingPendingFunctor_(false),
    epoller_(new Epoller(this)),
    wakeupfd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupfd_)),
    timerQueue_(new TimerQueue(this))
{
    if (t_loopInthisThread)
    {
        fprintf(stderr, "Anothre EventLoop exists in this thread!\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        t_loopInthisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupfd_);
    t_loopInthisThread = nullptr;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    
    while (!quit_)
    {
        activeChannels_.clear();
        Timestamp receiveTime = epoller_->epoll(1000, &activeChannels_);
        for (ChannelList::iterator it = activeChannels_.begin();
            it != activeChannels_.end(); ++it)
        {
            (*it)->handleEvent(receiveTime);
        }
        doPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        // 替换方案为使用 unique_ptr RAII 封装自动 lock AND unlock
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if (!isInLoopThread() || callingPendingFunctor_)
    {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    epoller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    epoller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assertInLoopThread();
    return epoller_->hasChannel(channel);
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    Timestamp time = addTime(Timestamp::now(), delay);
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double delay, TimerCallback cb)
{
    Timestamp time = addTime(Timestamp::now(), delay);
    return timerQueue_->addTimer(cb, time, delay);
}

void EventLoop::abortNotInLoopThread()
{
    fprintf(stderr, "abortNotInLoopThread()\n");
    exit(EXIT_FAILURE);
}

void EventLoop::wakeup()
{
    uint64_t on = 1;
    ssize_t size = ::write(wakeupfd_, &on, sizeof on);
    assert(size == sizeof on);
}

void EventLoop::handleRead()
{
    uint64_t on = 1;
    ssize_t size = ::read(wakeupfd_, &on, sizeof on);
    assert(size == sizeof on);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctor_ = true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor& func : functors)
    {
        func();
    }
    callingPendingFunctor_ = false;
}


