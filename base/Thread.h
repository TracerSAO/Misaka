#ifndef MISAKAK_THREAD_H
#define MISAKAK_THREAD_H

#include "Types.h"
#include "Atomic.h"
#include "noncopyable.h"
#include "CountDownLatch.h"

#include <functional>

namespace Misaka
{


class Thread : noncopyable
{
public:
	typedef std::function<void()> ThreadFunc;

public:
	explicit Thread(ThreadFunc func, const string& name = string());
	~Thread();

	void start();
	int join();		// return pthrad_join();

	bool started() const { return started_; }
	bool joined() const { return joined_; }
	pid_t tid() const { return tid_; }
	const string& name() const { return name_; }
	int numCreated() const { return numCreated_.get(); }

private:
	void setDefaultName();

private:
	bool started_;
	bool joined_;
	pthread_t pthreadId_;
	pid_t tid_;
	ThreadFunc func_;
	string name_;
	CountDownLatch latch_;

	static AtomicInt32 numCreated_;
};

}	// namespace Misaka

#endif // !MISAKAK_THREAD_H