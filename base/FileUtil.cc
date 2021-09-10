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
	// �趨���������ṩ�ַ���������д�� DISK
}

FileUtil::AppendFile::~AppendFile()
{
	::fclose(fp_);
}

void FileUtil::AppendFile::append(const char* logline, int len)
{
	size_t n = write(logline, len);
	size_t remain = len - n;
	while (0 < remain)	// �ظ����� write()��ȷ����������д��ɹ�
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
	// ʹ�õ������� fwrite -> thread unsafe
	// why? ��Ϊ Misaka ֻ��һ���� Disk д�� log data �� work thread
	// ��Ҳ�� muduo LOG ��������
	//		multi-thread �����ռ� every loop �в����� log data,
	//		����ֻ��һ���߳̽�����ȫ��д�� Disk
	// ���̵߳���д��־�����Ӧ�ã������Ƕ���߳�ȥдͬһ���ļ�������������ܿ죬�̼߳�ľ�̬ͬ���ᵼ�ºܴ�Ŀ�����������Ӱ��ʵʱ��
}
