#ifndef MISAKA_LOGFILE_H
#define MISAKA_LOGFILE_H

#include "Mutex.h"
#include "Types.h"
#include "FileUtil.h"

#include <memory>

namespace Misaka
{

//namespace FileUtil
//{
//	class AppendFile;
//	// namespace FileUtil
//}

class LogFile : noncopyable
{
public:
	LogFile(const string& basename,
		off_t rollSize,
		bool threadSafe = true,
		int flushInterval = 3,
		int checkEveryN = 1024);
	~LogFile();

	void append(const char* logline, int len);
	void flush();
	bool rollFile();

private:
	void append_unlocked(const char* logline, int len);

	static string getLogFileName(const string& basename, time_t* now);

private:
	const string basename_;	// LOG file 名字的一部分
	const off_t rollSize_;	// 一个 LOG file size 的最大值 [单位: byte]
	const int flushInterval_;	// LOG 开启强制 flush 的最长时间周期
	const int checkEveryN_;	// 程序一个周期内，可以向磁盘发起的写入操作的最大次数

	int count_;	// 记录当前已向磁盘发起写入的次数，当达到阈值后，强制启动 flush()，确保数据写入 DISK

	std::unique_ptr<MutexLock> mutex_;
	time_t startOfPeriod_;	// 初始期刊 -> 当前有效时间段 [单位: day]
	time_t lastRoll_;	// 保护 LOG file，避免出现重名，详见: rollFIle()
	time_t lastFlush_;	// 应对 rollSize 过大 && back-end 待写入数据过多的情况，确保数据可以及时 flush 进 DISK
	std::unique_ptr<FileUtil::AppendFile> file_;

	const static int kRollPerSeconds = 60 * 60 * 24;	// LOG roll(滚动) 的时间周期大小
};

}	// namespace Misaka

#endif // !MISAKA_LOGFILE_H
