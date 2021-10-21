#ifndef MISAKA_BUFFER_H
#define MISAKA_BUFFER_H

#include "../base/copyable.h"
#include "../base/StringPiece.h"
#include "../base/Types.h"

#include "EndianT.h"

#include <vector>
#include <algorithm>

#include <cassert>
#include <cstring>

namespace Misaka
{
namespace net
{
	
class Buffer : copyable
{
public:
	static const size_t kCheapPrepend = 8;
	static const size_t kInitialSize = 1024;

public:
	explicit Buffer(size_t initialSize = kInitialSize) :
		buffer_(kCheapPrepend + kInitialSize),
		readerIndex_(kCheapPrepend),
		writerIndex_(readerIndex_)
	{
		assert(0 == readableBytes());
		assert(kInitialSize == writableBytes());
		assert(kCheapPrepend == prependableBytes());
	}

	void swap(Buffer& buf)
	{
		buffer_.swap(buf.buffer_);
		readerIndex_ = buf.readerIndex_;
		writerIndex_ = buf.writerIndex_;
	}

	size_t readableBytes() const { return writerIndex_ - readerIndex_; }
	size_t writableBytes() const { return buffer_.size() - writerIndex_; }
	size_t prependableBytes() const { return readerIndex_; }

	const char* peek() const { return begin() + readerIndex_; }

	const char* findCRLF() const
	{
		const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}
	const char* findCRLF(const char* start) const
	{
		assert(start >= peek());
		assert(start <= beginWrite());
		const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}
	const char* findEOL() const
	{
		const void* eol = ::memchr(peek(), '\n', readableBytes());
		return static_cast<const char*>(eol);
	}
	const char* findEOL(const char* start) const
	{
		assert(start >= peek());
		assert(start <= beginWrite());
		const void* eol = ::memchr(start, '\n', beginWrite() - start);
		return static_cast<const char*>(eol);
	}

	void retrieve(size_t len)
	{
		assert(readableBytes() >= len);
		if (readableBytes() > len)
		{
			readerIndex_ += len;
		}
		else
		{
			retrieveAll();
		}
	}
	void retrieveUntil(const char* end)
	{
		assert(end >= peek());
		assert(end <= beginWrite());
		retrieve(end - peek());
	}
	void retrieveInt64()
	{
		retrieve(sizeof(int64_t));
	}
	void retrieveInt32()
	{
		retrieve(sizeof(int32_t));
	}
	void retrieve16()
	{
		retrieve(sizeof(int16_t));
	}
	void retrieve8()
	{
		retrieve(sizeof(int8_t));
	}
	void retrieveAll()
	{
		readerIndex_ = kCheapPrepend;
		writerIndex_ = kCheapPrepend;
	}
	string retrieveAllAsString()
	{
		return retrieveAsString(readableBytes());
	}
	string retrieveAsString(size_t len)
	{
		assert(readableBytes() >= len);
		string res(peek(), len);
		retrieve(len);
		return res;
	}
	StringPiece toStringPiece() const
	{
		return StringPiece(peek(), static_cast<int>(readableBytes()));
	}

	void append(const StringPiece& str)
	{
		append(str.data(), str.size());
	}
	void append(const char* data, size_t len)
	{
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}
	void append(const void* data, size_t len)
	{
		append(static_cast<const char*>(data), len);
	}

	void ensureWritableBytes(size_t len)
	{
		if (writableBytes() < len)
		{
			makeSpace(len);
		}
		assert(writableBytes() >= len);
	}
	char* beginWrite() { return begin() + writerIndex_; }
	const char* beginWrite() const { return begin() + writerIndex_; }
	void hasWritten(size_t len)
	{
		assert(writableBytes() >= len);
		writerIndex_ += len;
	}
	void unWrite(size_t len)
	{
		assert(readableBytes() >= len);
		writerIndex_ -= len;
	}

	// 以下被注释掉的 function，只有在需要序列化的情况才会用到，所以暂不实现
	//void appendInt64(int64_t x);
	//void appendInt32(int32_t x);
	//void appendInt16(int16_t x);
	//void appendInt8(int8_t x);

	//int64_t readInt64();
	//int32_t readInt32();
	//int16_t readInt16();
	//int8_t readInt8();

	//int64_t peekInt64() const;
	//int32_t peekInt32() const;
	//int16_t peekInt16() const;
	//int8_t peekInt8() const;

	// set prepend
	void prependInt64(int64_t x)
	{
		int64_t val64 = sockets::hostToNetwork64(x);
		prepend(&val64, sizeof(int64_t));
	}
	void prependInt32(int32_t x)
	{
		int32_t val32 = sockets::hostToNetwork32(x);
		prepend(&val32, sizeof(int32_t));
	}
	void prependInt16(int16_t x)
	{
		int16_t val16 = sockets::hostToNetwork16(x);
		prepend(&val16, sizeof(int16_t));
	}
	void prependInt8(int8_t x)
	{	// 大小端字节序问题，只存在于 data-type size > 1 byte
		prepend(&x, sizeof(int8_t));
	}

	void prepend(const void* data, size_t len)
	{
		assert(prependableBytes() >= len);
		readerIndex_ -= len;
		const char* v = static_cast<const char*>(data);
		std::copy(v, v + len, begin() + readerIndex_);
	}

	// 暂时没有什么比较好的替换方案
	//void shrink(size_t reserve);

	size_t internalCapacity() const { return buffer_.capacity(); }

	ssize_t readFd(int fd, int* savedErrno);

private:
	char* begin() { return buffer_.data(); }
	const char* begin() const { return buffer_.data(); }

	// 只能通过 ensureWritableByte() 来调用
	void makeSpace(size_t len)
	{
		if (prependableBytes() + writableBytes() < kCheapPrepend + len)
		{
			buffer_.resize(writerIndex_ + len);
		}
		else
		{
			// makeSpace() 不会被单独调用，能进来，说明 writableBytes() < len
			// 所以能进到 else ，说明 prependableBytes() != kCheapPrepend
			assert(kCheapPrepend < prependableBytes());
			size_t readablebytes = readableBytes();
			std::copy(begin() + readerIndex_,
					begin() + writerIndex_,
					begin() + kCheapPrepend);
			readerIndex_ = kCheapPrepend;
			writerIndex_ = readerIndex_ = readablebytes;
			assert(readableBytes() == readablebytes);
		}
	}

private:
	std::vector<char> buffer_;
	size_t readerIndex_;	// 指向最后一个 readByte 的下一位 -> [)
	size_t writerIndex_;	// 指向第一个 writeByte
	
	static const char kCRLF[];
};

}	// namespace net
}	// namespace Misaka


#endif // !MISAKA_BUFFER_H
