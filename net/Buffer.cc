#include "Buffer.h"

#include "SocketsOps.h"

#include <sys/uio.h>

#include <cerrno>

using namespace Misaka;
using namespace net;

const char Buffer::kCRLF[] = "\r\n";
const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno)
{
	char extrabuf[65536];
	const size_t writeable = writableBytes();
	struct iovec iov[2];
	iov[0].iov_base = beginWrite();
	iov[0].iov_len = writableBytes();
	iov[1].iov_base = extrabuf;
	iov[1].iov_len = sizeof extrabuf;
	const int iovcnt = writableBytes() < sizeof(extrabuf) ? 2 : 1;
	ssize_t n = sockets::readv(fd, iov, iovcnt);
	if (0 > n)
	{
		*savedErrno = errno;
	}
	else if (static_cast<size_t>(n) <= writeable)
	{
		writerIndex_ += n;
	}
	else
	{	// n > writableBytes()
		writerIndex_ = buffer_.size();
		append(extrabuf, n - writeable);
	}
	return n;
}