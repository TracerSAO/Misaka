#ifndef MISAKA_STRINGPIECE_H
#define MISAKA_STRINGPIECE_H

#include "Types.h"
#include <iosfwd>	// for ostream forward-declaration

/**
* �ṩ StringArg �� StringPiece ������ class ��Ŀ�ģ�
*	> �����ڴ����ַ�������ʱ���� std::string �Ĺ��죬������ string ��ʱ����
*	> ���򣬻������һ�� ctor��һ�� dtor���Լ� string ��ת�� �ַ�������
*/

namespace Misaka
{


//	�ṩ���ں����������͵� C-Style �� string �������� -> string-view
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

// �ṩ�����ں��������� CPP-Style �� string �������� -> string-view
class StringPiece	// : public copyable // ֵ����
{
public:
	StringPiece():
		ptr_(nullptr), length_(0) { }

	StringPiece(const char* str):
		ptr_(str), length_(static_cast<int>(strlen(ptr_))) { }
	StringPiece(const unsigned char* str):
		ptr_(reinterpret_cast<const char*>(str)),		// �޸���Ӱ�� sizeof(unsigned char) == sizeof(char)
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

	// FIXME: ����ʵ������׷�ӱ�����ʩ
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

// ���� StringPiece �� logged
std::ostream& operator<<(std::ostream& o, const Misaka::StringPiece& piece);

#endif // !MISAKA_STRINGPIECE_H
