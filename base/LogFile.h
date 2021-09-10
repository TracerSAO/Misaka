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
	const string basename_;	// LOG file ���ֵ�һ����
	const off_t rollSize_;	// һ�� LOG file size �����ֵ [��λ: byte]
	const int flushInterval_;	// LOG ����ǿ�� flush ���ʱ������
	const int checkEveryN_;	// ����һ�������ڣ���������̷����д�������������

	int count_;	// ��¼��ǰ������̷���д��Ĵ��������ﵽ��ֵ��ǿ������ flush()��ȷ������д�� DISK

	std::unique_ptr<MutexLock> mutex_;
	time_t startOfPeriod_;	// ��ʼ�ڿ� -> ��ǰ��Чʱ��� [��λ: day]
	time_t lastRoll_;	// ���� LOG file������������������: rollFIle()
	time_t lastFlush_;	// Ӧ�� rollSize ���� && back-end ��д�����ݹ���������ȷ�����ݿ��Լ�ʱ flush �� DISK
	std::unique_ptr<FileUtil::AppendFile> file_;

	const static int kRollPerSeconds = 60 * 60 * 24;	// LOG roll(����) ��ʱ�����ڴ�С
};

}	// namespace Misaka

#endif // !MISAKA_LOGFILE_H
