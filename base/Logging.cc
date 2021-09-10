#include "Logging.h"
#include "CurrentThread.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace Misaka
{

__thread char t_errnobuf[512];	// ���� errno ��Ӧ���쳣��Ϣ
__thread char t_time[64];		// ���浱ǰʱ�̵�ʱ����Ϣ - ΢��
__thread time_t t_lastsecond;	// ���浱ǰ 1 ���ʱ������Լ������ظ���¼�˿� 1 �����΢�벿�ֵ�ʱ���

const char* strerror_tl(int savedErrno)
{
	return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

// Ϊ Misaka ������ṩ���õ�ǰ���������������� LOG �������ķ�ʽ
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

// ��ȷ������Ϊ��class T �����ڱ���������� string ����
// ������Ϊ class T ���ⲿ�� LOGSTREAM �������ݵ�һ�����ýӿ�
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
	// muduo ����������Խ�����м�飬
	// ���������ˣ�������������������������������أ���
	// �Ƿ��жϳ�����������������ǣ�ֻ���������������ֹ�أ�
	// ��ʱ�����
	(void)n;
}

void defaultFlush()
{
	::fflush(stdout);	// ���ǵ��� libc func ����׼��� flush - ˢ��
}

Logger::OutputFunc g_output = defaultOutput;		// ��Ҳ�� "ͬ����־" �� "�첽��־" �л��ĺ��Ľӿ�
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

// �������ƣ�û�п���ʱ����Ŀǰ��ϵͳʱ����������в��㣬���Ҽ��£�����������
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
		// ��ʱ��ת��Ϊ struct tm ��ʽ���Ի�ȡ year month ������ [seconds -> year-mont-day hour-minute-second]

		int len = snprintf(t_time, sizeof t_time, "%4d%02d%02d %02d:%02d:%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
		// +1900 ����Ϊ�����ʱ���� 1900 �꿪ʼ���㣬��û�е� 0 ��һ˵
		// +1 ��ͬ����û�е� 0 ��һ˵
		assert(17 == len);	(void)len;
	}

	Fmt us(".%06dZ ", microseconds);
	assert(9 == us.length());
	stream_ << T(t_time, 17) << T(us.data(), 9);

	//stream_ << T(t_time, 17) << us;
	// ���� << �ֱ���ò�ͬ�� operator<< ����
	// ǰ�ߣ�operator<<(LogStream&, T)
	// ���ߣ�operator<<(LogStream&, Fmt)
	// ��ѡ�񳤵�˧���ǵĿ� :)
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