#include "EventLoop.h"
#include "Channel.h"
#include "Epoller.h"
#include "../base/Logging.h"

#include <poll.h>
#include <sys/epoll.h>
#include <cassert>

using namespace Misaka;
using namespace net;

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

Epoller::Epoller(EventLoop* loop):
	loop_(loop),
	epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
	events_(kInitEventListSize)
{
	int savedErrno = errno;
	if (0 < epollfd_)
	{
		LOG_TRACE << "Epoller::Epoller";
	}
	else if (0 == epollfd_)
	{
		// do-nothing
	}
	else
	{
		errno = savedErrno;
		LOG_SYSFATAL << "Epoller::Epoller";
	}
}

Timestamp Epoller::epoll(int timeoutMs, ChannelList* activeChannel)
{
	// 只会在固定的线程调用，不存在跨线程调用的情况
	int numEvents = ::epoll_wait(epollfd_,
									events_.data(),
									static_cast<int>(events_.size()),
									timeoutMs);
	int savedErrno = errno;
	Timestamp now(Timestamp::now());
	if (0 <= numEvents)
	{
		fillActiveChannels(numEvents, activeChannel);
		if (static_cast<size_t>(numEvents) == events_.size())
			events_.resize(2 * numEvents);
	}
	else
	{
		errno = savedErrno;
		if (EINTR != errno)
		{
			LOG_SYSERR << "Epoller::epoll";
		}
	}
	return now;
}

void Epoller::updateChannel(Channel* channel)
{
	loop_->assertInLoopThread();
	const int index = channel->index();
	if (kNew == index || kDeleted == index)
	{
		int fd = channel->fd();
		if (kNew == index)
		{
			assert(channels_.find(fd) == channels_.end());
			channels_[fd] = channel;
		}
		else
		{
			assert(channels_.find(fd) != channels_.end());
			assert(channels_[fd] == channel);
		}
		update(EPOLL_CTL_ADD, channel);
		channel->set_index(kAdded);
	}
	else
	{
		int fd = channel->fd();
		assert(channels_.find(fd) != channels_.end());
		assert(channels_[fd] == channel);
		if (channel->isNonEvent())
		{
			update(EPOLL_CTL_DEL, channel);
			channel->set_index(kDeleted);
		}
		else
		{
			update(EPOLL_CTL_MOD, channel);
		}
	}
}

void Epoller::removeChannel(Channel* channel)
{
	loop_->assertInLoopThread();
	int fd = channel->fd();
	assert(channels_.find(fd) != channels_.end());
	assert(channels_[fd] == channel);
	assert(channel->isNonEvent());		// 只有确定没有任何需要监听的事件后，才可以关闭
	const int index = channel->index();
	assert(kAdded == index || kDeleted == index);

	size_t n = channels_.erase(fd);
	assert(n == 1);

	if (kAdded == index)
	{
		update(EPOLL_CTL_DEL, channel);
	}
	channel->set_index(kNew);
}

bool Epoller::hasChannel(Channel* channel)
{
	int fd = channel->fd();
	return channels_.find(fd) != channels_.end() &&
			channels_[fd] == channel;
}

void Epoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
	// 只在当前线程调用，不存在跨线程调用的情况
	assert(static_cast<size_t>(numEvents) <= events_.size());
	for (int i = 0; i < numEvents; i++)
	{
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		channel->set_revents(events_[i].events);
		activeChannels->push_back(channel);
	}
}

void Epoller::update(int operation, Channel* channel)
{
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.ptr = channel;
	event.events = channel->events();
	if (::epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0)
	{
		if (EPOLL_CTL_DEL == operation)
		{
			LOG_SYSERR;
		}
		else
		{
			LOG_SYSFATAL;
		}
	}
}

