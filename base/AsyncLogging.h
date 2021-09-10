#ifndef MISAKA_ASYNCLOGGING_H
#define MISAKA_ASYNCLOGGING_H

#include "Mutex.h"
#include "Condition.h"
#include "CountDownLatch.h"
#include "Thread.h"
#include "noncopyable.h"
#include "LogStream.h"

#include <atomic>
#include <memory>
#include <vector>

namespace Misaka
{

class AsyncLogging : noncopyable
{
public:
	AsyncLogging(string basename,
				off_t rollSize,
				int flushInterval = 3);
	~AsyncLogging()
	{
		if (running_)
		{
			stop();
		}
	}

	void append(const char* str, int len);

	void start()
	{
		running_ = true;
		thread_.start();
		latch_.wait();
	}
	void stop()
	{
		running_ = false;
		cond_.notify();
		thread_.join();
	}

private:
	void threadFunc();	// 用于 thread 的回调函数

private:
	using Buffer = Misaka::detail::FixedBuffer<Misaka::detail::kLargeBuffer>;
	using BufferVector = std::vector<std::unique_ptr<Buffer>>;
	using BufferPtr = BufferVector::value_type;

	const int flushInterval_;
	std::atomic<bool> running_;
	const string basename_;
	const off_t rollSize_;
	Misaka::Thread thread_;
	Misaka::MutexLock mutex_;
	Misaka::Condition cond_;
	Misaka::CountDownLatch latch_;

	BufferPtr currentBuffer_;
	BufferPtr nextBuffer_;
	BufferVector buffers_;
};

}	// namespace Misaka;


#endif // !MISAKA_ASYNCLOGGING_H
