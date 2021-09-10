#include "Logging.h"
#include "CurrentThread.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace Misaka
{

__thread char t_errnobuf[512];	// 缓存 errno 对应的异常信息
__thread char t_time[64];		// 缓存当前时刻的时间信息 - 微秒
__thread time_t t_lastsecond;	// 缓存当前 1 秒的时间戳，以及避免重复记录此刻 1 秒大于微秒部分的时间戳

const char* strerror_tl(int savedErrno)
{
	return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

// 为 Misaka 网络库提供利用当前机器环境变量设置 LOG 输出级别的方式
Logger::LogLevel initLogLevel()
{
	if (::getenv("MISAKA_LOG_TRACE"))
		return Logger::LogLevel::TRACE;
	else if (::getenv("MISAKA_LOG_DEBUG"))
		return Logger::LogLevel::DEBUG;
	else
		return Logger::LogLevel::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[static_cast<int>(Logger::LogLevel::NUM_LOG_LEVELS)] =
{
	"TRACE ",
	"DEBUG ",
	"INFO  ",
	"WARN  ",
	"ERROR ",
	"FATAL "
};

// 我确定不认为，class T 可以在编译器计算出 string 长度
// 但我认为 class T 是外部向 LOGSTREAM 输入数据的一个良好接口
class T
{
public:
	T(const char* str, unsigned len) :
		str_(str),
		len_(len)
	{
		assert(strlen(str) == len);
	}

	const char* str_;
	const unsigned len_;
};

inline LogStream& operator<<(LogStream& os, T val)
{
	os.append(val.str_, val.len_);
	return os;
}

inline LogStream& operator<<(LogStream& os, const Logger::SourceFile& file)
{
	os.append(file.data_, file.size_);
	return os;
}

void defaultOutput(const char* msg, int len)
{
	size_t n = ::fwrite(msg, 1, len, stdout);
	// muduo 建议在这里对结果进行检查，
	// 但问题来了，如果检查出错误，那这个错误忘哪里输出呢？？
	// 是否中断程序并输出错误结果，还是？只输出错误结果程序不终止呢？
	// 暂时不检查
	(void)n;
}

void defaultFlush()
{
	::fflush(stdout);	// 就是调用 libc func 将标准输出 flush - 刷新
}

Logger::OutputFunc g_output = defaultOutput;		// 这也是 "同步日志" 和 "异步日志" 切换的核心接口
Logger::FlushFunc g_flush = defaultFlush;

}	// namespace Misaka


using namespace Misaka;

Logger::Impl::Impl(LogLevel level, int old_errno, const Logger::SourceFile& basename, int line):
	stream_(),
	time_(Timestamp::now()),
	level_(level),
	line_(line),
	basename_(basename)
{
	// No1. time - [YearMonthDay Hour:Minute:Second.MicroSeconds]
	formatTime();
	// No2. thread
	stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
	// No3. Level
	stream_ << T(LogLevelName[static_cast<int>(level_)], 6);
	// No4. log data
	if (0 != old_errno)
	{
		stream_ << strerror_tl(old_errno) << "(errno = " << old_errno << ") ";
	}
}

// 这里的设计，没有考虑时区，目前对系统时间的掌握稍有不足，暂且记下，后续再完善
void Logger::Impl::formatTime()
{
	int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
	time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
	int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
	if (t_lastsecond != seconds)
	{
		t_lastsecond = seconds;
		struct tm tm_time;
		::gmtime_r(&seconds, &tm_time);
		// 将时间转换为 struct tm 格式，以获取 year month 等数据 [seconds -> year-mont-day hour-minute-second]

		int len = snprintf(t_time, sizeof t_time, "%4d%02d%02d %02d:%02d:%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
		// +1900 是因为计算机时间是 1900 年开始计算，年没有第 0 年一说
		// +1 月同样是没有第 0 月一说
		assert(17 == len);	(void)len;
	}

	Fmt us(".%06dZ ", microseconds);
	assert(9 == us.length());
	stream_ << T(t_time, 17) << T(us.data(), 9);

	//stream_ << T(t_time, 17) << us;
	// 两个 << 分别调用不同的 operator<< 重载
	// 前者：operator<<(LogStream&, T)
	// 后者：operator<<(LogStream&, Fmt)
	// 我选择长得帅的那的款 :)
}

void Logger::Impl::finish()
{
	stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line):
	impl_(LogLevel::INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level) :
	impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func) :
	impl_(level, 0, file, line)
{
	impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, bool toAbort) :
	impl_(toAbort ? LogLevel::FATAL : LogLevel::ERROR, errno, file, line)
{
}

Logger::~Logger()
{
	impl_.finish();
	const LogStream::Buffer& buf(stream().buffer());
	g_output(buf.data(), buf.length());
	if (Logger::LogLevel::FATAL == impl_.level_)
	{
		g_flush();
		abort();
	}
}

void Logger::setLogLevel(Logger::LogLevel level)
{
	g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
	g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
	g_flush = flush;
}