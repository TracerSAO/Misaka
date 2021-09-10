#ifndef MISAKA_STRINGPIECE_H
#define MISAKA_STRINGPIECE_H

#include "Types.h"
#include <iosfwd>	// for ostream forward-declaration

/**
* 提供 StringArg 和 StringPiece 这两个 class 的目的：
*	> 避免在传递字符串常量时，因 std::string 的构造，而产生 string 临时对象
*	> 否则，会多增加一次 ctor、一次 dtor、以及 string 会转存 字符串变量
*/

namespace Misaka
{


//	提供用于函数参数类型的 C-Style 的 string 参数类型 -> string-view
class StringArg
{
public:
	StringArg(const char* str) :
		str_(str)
	{ }
	StringArg(const string& str) :
		str_(str.c_str())
	{ }

	const char* c_str() const
	{ return str_; }

private:
	const char* str_;
};

// 提供可用于函数参数的 CPP-Style 的 string 参数类型 -> string-view
class StringPiece	// : public copyable // 值语义
{
public:
	StringPiece():
		ptr_(nullptr), length_(0) { }

	StringPiece(const char* str):
		ptr_(str), length_(static_cast<int>(strlen(ptr_))) { }
	StringPiece(const unsigned char* str):
		ptr_(reinterpret_cast<const char*>(str)),		// 无负面影响 sizeof(unsigned char) == sizeof(char)
		length_(static_cast<int>(strlen(ptr_)))	{ }
	StringPiece(const string& str) :
		ptr_(str.c_str()), length_(static_cast<int>(str.size())) { }
	StringPiece(const char* ptr, int length):
		ptr_(ptr), length_(length) { }

	const char* data() const { return ptr_; }
	int size() const { return length_; }
	bool empty() const { return 0 == length_; }
	const char* begin() const { return ptr_; }
	const char* end() const { return ptr_ + length_; }

	void clear() {
		ptr_ = nullptr;
		length_ = 0;
	}
	void set(const char* buffer, int len) {
		ptr_ = buffer;
		length_ = len;
	}
	void set(const char* str) {
		ptr_ = str;
		length_ = static_cast<int>(strlen(str));
	}
	void set(const void* buffer, int len) {
		ptr_ = static_cast<const char*>(buffer);
		length_ = len;
	}

	char operator[](int i) const {
		return ptr_[i];
	}

	// FIXME: 根据实际需求，追加保护措施
	void remove_prefix(int n) {
		ptr_ += n;
		length_ -= n;
	}

	void remove_suffix(int n) {
		length_ -= n;
	}

	bool operator==(const StringPiece& x) const {
		return length_ == x.length_ &&
			(memcmp(ptr_, x.ptr_, length_) == 0);
	}
	bool operator!=(const StringPiece& x) const {
		return !(*this == x);
	}

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)									\
	bool operator cmp (const StringPiece& x) const {								\
		int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_: x.length_);		\
		return ((r auxcmp 0) || ((0 == r) && (length_ auxcmp x.length_)));			\
	}
	STRINGPIECE_BINARY_PREDICATE(<, <);
	STRINGPIECE_BINARY_PREDICATE(<=, <);
	STRINGPIECE_BINARY_PREDICATE(>, >);
	STRINGPIECE_BINARY_PREDICATE(>=, >);
#undef STRINGPIECE_BINARY_PREDICATE

	int compare(const StringPiece& x) const {
		int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
		if (0 == r) {
			if (length_ < x.length_) r = -1;
			else if (length_ > x.length_) r = +1;
		}
		return r;
	}

	string as_string() const {
		return string(ptr_, length_);
	}

	void CopyToString(string* target) const {
		target->assign(ptr_, length_);
	}

	bool starts_with(const StringPiece& x) const {
		return (length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0);
	}

private:
	const char* ptr_;
	int length_;
};


}	// namespace Misaka

// 允许 StringPiece 被 logged
std::ostream& operator<<(std::ostream& o, const Misaka::StringPiece& piece);

#endif // !MISAKA_STRINGPIECE_H
