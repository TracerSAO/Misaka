#ifndef MISAKA_EventLoop_H
#define MISAKA_EventLoop_H

#include "callbacks.h"
#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "../base/Mutex.h"

#include <vector>
#include <memory>

#include <pthread.h>

namespace Misaka
{
namespace net
{

class Channel;
class Epoller;
class TimerId;
class TimerQueue;

class EventLoop : noncopyable
{
public:
    typedef std::function<void()> Functor;
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void assertInLoopThread()
    {
        if (!isInLoopThread())
            abortNotInLoopThread();
    }
    bool isInLoopThread() const
    {
        return threadId_ == pthread_self();
    }

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    TimerId runAt(Timestamp time, TimerCallback cb);
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runEvery(double delay, TimerCallback cb);

private:
    void abortNotInLoopThread();
    void handleRead();
    void doPendingFunctors();
    void wakeup();

private:
    typedef std::vector<Channel*> ChannelList;

    bool quit_;
    bool looping_; /*atomic - 表示对 ta 的一切操作一定都是原子的*/
    const pthread_t threadId_;
    bool callingPendingFunctor_; /*atomic*/

    std::unique_ptr<Epoller> epoller_;
    int wakeupfd_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::unique_ptr<TimerQueue> timerQueue_;

    ChannelList activeChannels_;

    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;
};

}   // namespace net
}   // namespace Misaka

#endif // EventLoop.h