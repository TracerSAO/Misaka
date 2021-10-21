#include "Thread.h"

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <unistd.h>

namespace Misaka
{
namespace Detail
{

pid_t gettid()
{
	return static_cast<pid_t>(::syscall(SYS_gettid));
	// 这里是 muduo 封装 thread 的一个核心，用于唯一表示 thread 的标识符
	// 使用 ::syscall(SYS_gettid) 目的是获取，当前线程在运行时，Linux-kernel 为其分配的标示性等同于 “进程唯一标识符” 的 pid_t
	// pid_t 会比 pthread_t 更好，原因就是 “pid_t 标识范围可跨进程 | pthread_t 标识范围只能在当前进程内，甚至不同进程内的不同线程会存在 pthread_t 冲突的情况”
	// 最后，这一切的目的都是为：LOG 服务，让我们在程序 abort 后，检验“尸体”能够更轻松一点 ;)
}

void afterfork()
{
	CurrentThread::t_cachedTid = 0;
	CurrentThread::t_threadName = "main";
	CurrentThread::tid();
}

class ThreadNameInitializer {
public:
	ThreadNameInitializer() {
		CurrentThread::t_threadName = "main";
		CurrentThread::tid();
		pthread_atfork(nullptr, nullptr, afterfork);
	}
};

ThreadNameInitializer init;

struct ThreadData
{
	typedef Misaka::Thread::ThreadFunc ThreadFunc;

	ThreadFunc func_;
	string name_;
	pid_t* tid_;
	CountDownLatch* latch_;

	ThreadData(ThreadFunc func,
		const string& n,
		pid_t* tid,
		CountDownLatch* latch) :
		func_(func),
		name_(n),
		tid_(tid),
		latch_(latch)
	{ }

	void runInThread()
	{
		// 1. 新线程诞生后的基础工作 —— “按照我们自己设计的 thread 模型” 对 Linux 创建的原生线程进行初始化工作
		*tid_ = CurrentThread::tid();
		tid_ = nullptr;
		latch_->countDown();
		latch_ = nullptr;

		CurrentThread::t_threadName = name_.empty() ? "MisakaThread" : name_.c_str();
		::prctl(PR_SET_NAME, CurrentThread::t_threadName);

		// 2. 初始化工作完毕，开始让 thread 从事 TA 的本质工作，运行调用者注册进来的 function
		func_();	// 缺少异常捕获，为什么要在这里添加异常捕获，暂不清楚，muduo 中在这一块确实提供了这样的一种机制
		CurrentThread::t_threadName = "finished";
	}
};

void* startThread(void* arg)
{
	auto* data = static_cast<ThreadData*>(arg);
	data->runInThread();
	delete data;
	return nullptr;
}

}	// namespace Detail

void CurrentThread::cachedTid()
{
	if (0 == t_cachedTid)
	{
		t_cachedTid = Detail::gettid();
		t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
	}
}

bool CurrentThread::isMainThread()
{
	return ::getpid() == tid();
}

AtomicInt32 Thread::numCreated_;

Thread::Thread(ThreadFunc func, const string& n) :
	started_(false),
	joined_(false),
	pthreadId_(0),
	tid_(0),
	func_(func),
	name_(n),
	latch_(1)
{
	setDefaultName();
}

Thread::~Thread()
{
	if (started_ && !joined_)
	{
		pthread_detach(pthreadId_);
	}
}

void Thread::setDefaultName()
{
	int num = numCreated_.incrementAndGet();
	if (name_.empty())
	{
		char buf[32];
		bzero(buf, sizeof buf);
		snprintf(buf, sizeof buf, "Thread%d", num);
		name_ = buf;
	}
}

void Thread::start()
{
	started_ = true;
	auto* data = new Detail::ThreadData(func_, name_, &tid_, &latch_);
	if (pthread_create(&pthreadId_, nullptr, &Detail::startThread, static_cast<void*>(data)))
	{
		started_ = false;
		delete data;
		assert(true == started_);	// 暂替 LOG
	}
	else
	{
		latch_.wait();	// 等待 thread 创建完毕，并开始执行 threadFunc
		assert(0 < tid_);
	}

}

int Thread::join()
{
	assert(started_);
	assert(!joined_);
	joined_ = true;
	return pthread_join(pthreadId_, nullptr);
}

}	// namespace Misaka