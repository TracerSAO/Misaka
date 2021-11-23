#ifndef MISAKA_TIMINGWHEEL_H
#define MISAKA_TIMINGWHEEL_H

#include <boost/circular_buffer.hpp>    // boost 库中提供的可固定大小，循环容器
#include <memory>
#include <unordered_set>

#include "../base/copyable.h"
#include "../base/noncopyable.h"
#include "../base/Mutex.h"
#include "../base/Logging.h"

#include "../net/TcpConnection.h"

namespace Misaka
{
namespace net
{
namespace http
{

class TimingWheel : noncopyable
{
public:
	typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
	struct Entry : public copyable
	{
		explicit Entry(const WeakTcpConnectionPtr& weakconn) :
			weakConn_(weakconn)
		{ }

		~Entry()
		{
			std::shared_ptr<TcpConnection> conn = weakConn_.lock();
			if (conn)
			{
				//conn->shutdown();
				conn->forceClose();
				LOG_INFO << "forceClose over";
			}
		}

		WeakTcpConnectionPtr weakConn_;
	};
	typedef std::shared_ptr<Entry> EntryPtr;
	typedef std::weak_ptr<Entry> WeakEntryPtr;
	typedef std::unordered_set<EntryPtr> Bucket;
	typedef boost::circular_buffer<Bucket> WeakConnectionList;

public:
	TimingWheel(int bucketSize = 10) :
		mutex_(),
		connectionBuckets_(bucketSize)
	{
		connectionBuckets_.resize(bucketSize);
	}

	//	定时事件
	//	How-to-use: HTTPServer 模块将其注册为 “循环定时事件”
	//  Effect: interval 1 s，触发一次
	//	Default-Action：插入新的容器元素
	//	PS：可根据 HTTPServer 需求再 HTTPServer 模块中重新更改定时 interval
	void onTimer()
	{
		// string str;
		// for (int i = 0; i < static_cast<int>(connectionBuckets_.size()); i++)
		// {
		// 	int val = static_cast<int>(connectionBuckets_[i].size()) + static_cast<int>('0');
		// 	str.push_back(static_cast<char>(val));
		// }
		// LOG_INFO << "connectionBuckets: " << str;
		
		MutexLockGuard GUARD(mutex_);
		connectionBuckets_.push_back(Bucket());
	}

	// How-to-use:	在 HttpServer 执行对 TCPConnection 的 Read/Write 操作时，附加直接此状态更新操作
	// Effect:	为 TcpConnection 续命 ;)
	// Default-Action: 想 circular_buffer 末尾的元素中插入代表链接的 Entry
	void push_backEntryPtr(const EntryPtr& entryPtr)
	{
		MutexLockGuard GUARD(mutex_);
		connectionBuckets_.back().insert(entryPtr);
	}

	bool containsInBackBucket(const EntryPtr& entryPtr) const
	{
		MutexLockGuard GUARD(mutex_);
		return connectionBuckets_.back().find(entryPtr) !=
			connectionBuckets_.back().end();
	}

private:
	mutable MutexLock mutex_;
	WeakConnectionList connectionBuckets_;
};

}	// namespace http
}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_TIMINGWHEEL_H
