#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"

#include <poll.h>

#include <cassert>

using namespace Misaka;
using namespace net;

const int Channel::kNoneEvent = POLLNVAL;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd):
    loop_(loop),
    fd_(fd),
    events_(0),
    revent_(0),
    index_(-1),
    tied_(false),
    addToLoop_(false),
    eventHandling_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);    // 很重要的判断，就我 doge life，是对生命周期的保证，绝对 一系列回调过程中重要对象 没有被析构，eg: TcpConnectoin
    assert(!addToLoop_);    // 判断 自己 -> loop
    if (loop_->isInLoopThread())
    {
        assert(!loop_->hasChannel(this));   // 判断 自己 <- loop
    }
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::remove()
{
    assert(isNonEvent());
    addToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::update()
{
    addToLoop_ = true;
    loop_->updateChannel(this);
}

// Timestamp 参数由 EventLoop::loop() -> Poller::poll() return Timestamp::now()
void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;    // 守门人 - 生命周期的守护者
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_TRACE << loop_ << " self: " << this;
    eventHandling_ = true;

    if ((revent_ & POLLHUP) && !(revent_ & POLLIN))
    {
        LOG_WARN << "fd = " << fd_ << "Channel::handleEvnet() POLLHUP";
        if (closeCallback_) closeCallback_();
    }

    if (revent_ & POLLNVAL)
    {
        LOG_WARN << "fd = " << fd_ << "Channel::handleEvent() POLLINVAL";
    }

    if (revent_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }
    if (revent_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revent_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }

    eventHandling_ = false;
    LOG_TRACE << loop_ << " self: " << this;
}
