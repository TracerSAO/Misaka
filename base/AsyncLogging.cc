#include "AsyncLogging.h"
#include "LogFile.h"
#include "Timestamp.h"

using namespace Misaka;

AsyncLogging::AsyncLogging(const string basename,
							off_t rollSize,
							int flushInterval):
	flushInterval_(flushInterval),
	running_(false),
	basename_(basename),
	rollSize_(rollSize),
	thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
	mutex_(),
	cond_(mutex_),
	latch_(1),
	currentBuffer_(new Buffer),
	nextBuffer_(new Buffer),
	buffers_()
{
	currentBuffer_->bzero();
	nextBuffer_->bzero();
	buffers_.reserve(16);		// 最多存放 16 块 Buffer 缓存
}

void AsyncLogging::append(const char* logline, int len)
{
	MutexLockGuard MG(mutex_);
	if (currentBuffer_->avail() >= len)
	{
		currentBuffer_->append(logline, len);
	}
	else
	{
		buffers_.push_back(std::move(currentBuffer_));	// unique_ptr 特殊的性质，只能使用移动语义
		if (nullptr != nextBuffer_)
		{
			currentBuffer_ = std::move(nextBuffer_);
		}
		else
		{
			currentBuffer_.reset(new Buffer);
		}
		currentBuffer_->append(logline, len);
		cond_.notify();
	}
}

void AsyncLogging::threadFunc()
{
	assert(true == running_);
	latch_.countDown();	// 通知主线程，自己已经执行到 threadFunc()
	LogFile output(basename_, rollSize_, false);
	BufferPtr nextBuffer1(new Buffer);
	BufferPtr nextBuffer2(new Buffer);
	nextBuffer1->bzero();
	nextBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	while (running_)
	{
		assert(nextBuffer1 && 0 == nextBuffer1->length());
		assert(nextBuffer2 && 0 == nextBuffer2->length());
		assert(buffersToWrite.empty());

		{
			MutexLockGuard MG(mutex_);
			if (buffers_.empty())
			{
				cond_.waitForSeconds(flushInterval_);
			}
			buffers_.push_back(std::move(currentBuffer_));
			currentBuffer_ = std::move(nextBuffer1);
			buffersToWrite.swap(buffers_);
			if (!nextBuffer_)
			{
				nextBuffer_ = std::move(nextBuffer2);
			}
		}

		if (buffersToWrite.size() > 25)
		{
			char buf[256];
			snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
				Timestamp::now().toFormattedString().c_str(),
				buffersToWrite.size() - 2);
			fputs(buf, stderr);
			output.append(buf, static_cast<int>(sizeof buf));
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
		}

		for (const BufferPtr& buffer : buffersToWrite)
		{
			output.append(buffer->data(), buffer->length());
		}

		if (2 < buffersToWrite.size())
		{
			buffersToWrite.resize(2);
		}

		if (!nextBuffer1)
		{
			nextBuffer1 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			nextBuffer1->reset();
		}
		if (!nextBuffer2)
		{
			nextBuffer2 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			nextBuffer2->reset();
		}

		buffersToWrite.clear();
		output.flush();
	}
	output.flush();
}