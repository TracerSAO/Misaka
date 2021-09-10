#ifndef MISAKA_EPOLLER_H
#define MISAKA_EPOLLER_H

#include "../base/Timestamp.h"

#include <vector>
#include <map>

struct epoll_event;

namespace Misaka
{
namespace net
{

class Channel;
class EventLoop;

class Epoller {
public:
	typedef std::vector<Channel*> ChannelList;

public:
	Epoller(EventLoop* loop);

	Timestamp epoll(int timeoutMs, ChannelList* activeChannel);
	void updateChannel(Channel* channel);
	void removeChannel(Channel* channel);
	bool hasChannel(Channel* channel);

private:
	void fillActiveChannels(int numEvents,
		ChannelList* activeChannels) const;
	void update(int operation, Channel* channel);

private:
	typedef std::vector<struct epoll_event> EventList;
	typedef std::map<int, Channel*> ChannelMap;

	const int kInitEventListSize = 16;

	EventLoop* loop_;
	int epollfd_;
	EventList events_;
	ChannelMap channels_;
};

}	// namespace net
}	// namespace Misaka

#endif  // Epoller.h