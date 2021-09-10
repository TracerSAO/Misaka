#ifndef MISAKA_CHANNEL_H
#define MISAKA_CHANNEL_H

#include "../base/Timestamp.h"

#include <memory>
#include <functional>

namespace Misaka
{
namespace net
{

class EventLoop;

class Channel {
public:
    typedef std::function<void()> EventCalback;
    typedef std::function<void (Timestamp)> ReadEventCallback;

public:
    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(Timestamp);

    void setReadCallback(ReadEventCallback cb)
    {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCalback cb)
    {
        writeCallback_ = std::move(cb);
    }
    void setCloseCallback(EventCalback cb)
    {
        closeCallback_ = std::move(cb);
    }
    void setErrorCallback(EventCalback cb)
    {
        errorCallback_ = std::move(cb);
    }

    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revent_ = revt; }
    bool isNonEvent() const { return events_ == kNoneEvent; }

    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    // for Poller
    int index() { return index_; }
    void set_index(int index) { index_ = index; }

    EventLoop* ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);


private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int  fd_;
    int events_;
    int revent_;
    int index_;     // 记录自己在 poller 中的位置

    std::weak_ptr<void> tie_;
    bool tied_;
    bool addToLoop_;
    bool eventHandling_;
    ReadEventCallback readCallback_;
    EventCalback writeCallback_;
    EventCalback errorCallback_;
    EventCalback closeCallback_;
};

}   // namespace net
}   // namespace Misaka

#endif  // Channel.h