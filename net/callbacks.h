#ifndef MISAKA_CALLBACK_H
#define MISAKA_CALLBACK_H

#include "../base/Timestamp.h"
#include "Buffer.h"

#include <functional>
#include <memory>

namespace Misaka
{
namespace net
{

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

template <typename T>
T* get_pointer(std::shared_ptr<T>& ptr)
{
	return ptr.get();
}

template <typename T>
T* get_pointer(std::unique_ptr<T>& ptr)
{
	return ptr.get();
}

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void()> TimerCallback;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;	// 数据量超过缓存警戒线 -> 触发高水位事件

// 不同于其他几个 callback，新增加的 Timestamp 是提供了一个 info —— 消息何时到达的 info，
// 用不用，就要看 user 自己
typedef std::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;


void defaultConnectionCallback(const TcpConnectionPtr&);
void defaultMessageCallback(const TcpConnectionPtr&, Buffer*, Timestamp);

}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_CALLBACK_H
