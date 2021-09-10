#ifndef MISAKA_LOGGING_H
#define MISAKA_LOGGING_H

#include "Timestamp.h"
#include "LogStream.h"

namespace Misaka
{

class Logger
{
public:
enum class LogLevel : int
{
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERROR,
	FATAL,
	NUM_LOG_LEVELS
};

class SourceFile
{
public:
	template <int SIZE>
	SourceFile(const char(&arr)[SIZE]):
		data_(arr),
		size_(SIZE-1)	// -1 -> '\0'
	{
		const char* lastIndex = strrchr(data_, '/');
		if (lastIndex)
		{
			data_ = lastIndex + 1;		// [abc/file] -> abc/ [file]
			size_ -= static_cast<int>(data_ - arr);
		}
	}

	explicit SourceFile(const char* filename):
		data_(filename)
	{
		const char* lastIndex = strrchr(data_, '/');
		if (lastIndex)
		{
			data_ = lastIndex + 1;
		}
		size_ = static_cast<int>(strlen(data_));
	}

	const char* data_;
	int size_;
};

	Logger(SourceFile file, int line);
	Logger(SourceFile file, int line, LogLevel level);
	Logger(SourceFile file, int line, LogLevel level, const char* func);
	Logger(SourceFile file, int line, bool toAbort);
	~Logger();

	LogStream& stream() { return impl_.stream_; }

	static LogLevel logLevel();
	static void setLogLevel(LogLevel level);
	
	using OutputFunc = void(*)(const char* msg, int len);
	using FlushFunc = void(*)();
	static void setOutput(OutputFunc);
	static void setFlush(FlushFunc);

private:
// Impl class 存在的目的是辅助 Logger，主要处理 log 数据的格式,
// 这样 Logger class 可以只关注与 LOG 相关功能的对外接口
class Impl
{
public:
	using LogLevel = Logger::LogLevel;
	Impl(LogLevel level, int old_errno, const SourceFile& basename, int line);

	void formatTime();
	void finish();

	LogStream stream_;
	Timestamp time_;
	LogLevel level_;
	int line_;
	SourceFile basename_;
};

	Impl impl_;
};

// g_logLevel 代表的是整个 Logger 工作时的 MIN log-level
extern Logger::LogLevel g_logLevel;	// Logging.cc
inline Logger::LogLevel Logger::logLevel()
{
	return g_logLevel;
}

#define LOG_TRACE if (Misaka::Logger::logLevel() <= Misaka::Logger::LogLevel::TRACE) \
	Misaka::Logger(__FILE__, __LINE__, Misaka::Logger::LogLevel::TRACE, __func__).stream()
#define LOG_DEBUG if (Misaka::Logger::logLevel() <= Misaka::Logger::LogLevel::DEBUG) \
	Misaka::Logger(__FILE__, __LINE__, Misaka::Logger::LogLevel::DEBUG, __func__).stream()
#define LOG_INFO if (Misaka::Logger::logLevel() <= Misaka::Logger::LogLevel::INFO) \
	Misaka::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Misaka::Logger(__FILE__, __LINE__, Misaka::Logger::LogLevel::WARN).stream()
#define LOG_ERROR Misaka::Logger(__FILE__, __LINE__, Misaka::Logger::LogLevel::ERROR).stream()
#define LOG_FATAL Misaka::Logger(__FILE__, __LINE__, Misaka::Logger::LogLevel::FATAL).stream()
#define LOG_SYSERR Misaka::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Misaka::Logger(__FILE__, __LINE__, true).stream()
// 1. LOG_INFO 不需要外部输入 INFO，原因在于 Logging.cc， initLogLevel() 设定了 default LogLevel = INFO
// 2. Logger(__FILE__, __LINE__, xxx); 中 __FILE__ 为 const char*，
//		因为没有显示(explicit)调用SourceFile 的缘故，会导致 SourceFile 调用模板推导的那个 ctor
//		若仍有疑惑，可自行编写简单的测试程序，一探究竟

// 对外提供，一个 strerror_r 获取 errorno 错误信息的可冲入函数的封装
const char* strerror_tl(int savedErrno);

#define CHECKNOTNULL(val) \
	::Misaka::CheckNotNull(__FILE__, __LINE__, "'"#val"' Must be not null", (val))

// 提供 nullptr 指针检查机制，内部封装 LOG 输出异常结果
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr)
{
	if (nullptr == ptr)
	{
		Logger(file, line, Logger::LogLevel::FATAL).stream() << names;
	}
	return ptr;
}

}	// namespace Misaka

#endif // !MISAKA_LOGGING_H
