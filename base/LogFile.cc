#include "LogFile.h"
#include "FileUtil.h"
#include "ProcessInfo.h"

#include <cassert>
#include <cstdlib>

using namespace Misaka;

LogFile::LogFile(const string& basename,
				off_t rollSize,
				bool threadSafe,
				int flushInterval,
				int checkEveryN) :
	basename_(basename),
	rollSize_(rollSize),
	flushInterval_(flushInterval),
	checkEveryN_(checkEveryN),
	count_(0),
	mutex_(threadSafe ? new MutexLock : nullptr),
	startOfPeriod_(0),
	lastRoll_(0),
	lastFlush_(0)
{
	assert(string::npos == basename_.find('/'));
	rollFile();
	// 初始化 lastRoll_ lastFlush_ startOfPeriod_
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len)
{
	if (mutex_)
	{
		MutexLockGuard MG(*mutex_);
		append_unlocked(logline, len);
	}
	else
	{
		append_unlocked(logline, len);
	}
}

void LogFile::flush()
{
	if (mutex_)
	{
		MutexLockGuard MG(*mutex_);
		file_->flush();
	}
	else
	{
		file_->flush();
	}
}

bool LogFile::rollFile()
{
	time_t now = 0;
	string logfilename = getLogFileName(basename_, &now);
	time_t start = now / kRollPerSeconds * kRollPerSeconds;

	if (now > lastRoll_)
	{
		lastRoll_ = now;
		lastFlush_ = now;
		startOfPeriod_ = start;
		 file_.reset(new FileUtil::AppendFile(logfilename));
		return true;
	}
	return false;
}

void LogFile::append_unlocked(const char* logline, int len)
{
	file_->append(logline, len);
	
	if (file_->writtenBytes() > rollSize_)
	{
		rollFile();
	}
	else
	{
		count_++;
		if (checkEveryN_ <= count_)
		{
			count_ = 0;
			time_t now = ::time(nullptr);
			time_t nowOfPeriod = now / kRollPerSeconds * kRollPerSeconds;
			if (nowOfPeriod != startOfPeriod_)
			{
				rollFile();
			}
			else if (now - lastFlush_ > flushInterval_)
			{
				lastFlush_ = now;
				file_->flush();
			}
		}
	}
}

string LogFile::getLogFileName(const string& basename, time_t* now)
{
	// Logfilename format:
	// [程序名] . [year-month-day] - [hour-minute-second] . [hostname] . [pid_t] .log

	string logfilenam;
	logfilenam.reserve(basename.size() + 64);
	logfilenam = basename;

	*now = ::time(nullptr);
	struct tm tm_now;
	::gmtime_r(now, &tm_now);

	// Y-m-d-h-m-s -> 20210722-102442
	char timebuf[32];
	strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm_now);
	logfilenam += timebuf;

	// hostname 
	logfilenam += ProcessInfo::hostname();

	// pid
	logfilenam += '.' + ProcessInfo::pidString();

	logfilenam += ".log";

	return logfilenam;
}
