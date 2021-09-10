#ifndef MISAKA_FILEUTIL_H
#define MISAKA_FILEUTIL_H

#include "noncopyable.h"
#include "StringPiece.h"
#include "Types.h"

#include <cstdio>

namespace Misaka
{
namespace FileUtil
{

// it's my
class AppendFile : noncopyable
{
public:
	AppendFile(StringArg filename);
	~AppendFile();

	void append(const char* logline, int line);
	void flush();

	off_t writtenBytes() const { return writtenBytes_; }

private:
	size_t write(const char* str, size_t len);

private:
	FILE* fp_;
	char buffer_[64 * 1024];
	off_t writtenBytes_;
};

}	// namespace FileUtil
}	// namespace Misaka

#endif // !MISAKA_FILEUTIL_H