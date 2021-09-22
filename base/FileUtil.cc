#include "FileUtil.h"
#include "Logging.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>

using namespace Misaka;

FileUtil::AppendFile::AppendFile(StringArg filename) :
	fp_(::fopen(filename.c_str(), "ae")),
	writtenBytes_(0)
{
	assert(fp_);
	::setbuffer(fp_, buffer_, sizeof buffer_);
	// 设定缓冲区，提供字符内容批量写入 DISK
}

FileUtil::AppendFile::~AppendFile()
{
	::fclose(fp_);
}

void FileUtil::AppendFile::append(const char* logline, int len)
{
	size_t n = write(logline, len);
	size_t remain = len - n;
	while (0 < remain)	// 重复调用 write()，确保所有数据写入成功
	{
		size_t x = write(logline + n, remain);
		if (0 == x)
		{
			int err = ferror(fp_);
			if (err)
			{
				fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
			}
			break;
		}
		n += x;
		remain -= x;
	}

	writtenBytes_ += len;
}

void FileUtil::AppendFile::flush()
{
	::fflush(fp_);
}

size_t FileUtil::AppendFile::write(const char* str, size_t len)
{
	return ::fwrite_unlocked(str, 1, len, fp_);
	// 使用的是无锁 fwrite -> thread unsafe
	// why? 因为 Misaka 只有一个向 Disk 写入 log data 的 work thread
	// 这也是 muduo LOG 的设计理念：
	//		multi-thread 用于收集 every loop 中产生的 log data,
	//		最终只有一个线程将数据全数写入 Disk
	// 多线程的在写日志方面的应用，并不是多个线程去写同一个文件，这样并不会很快，线程间的竞态同样会导致很大的开销，甚至会影响实时性
}
