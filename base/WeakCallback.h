#ifndef MISAKA_WEAKCALLBACK_H
#define MISAKA_WEAKCALLBACK_H

#include <memory>
#include <functional>
#include "copyable.h"

namespace Misaka
{

template <typename CLASS, typename... ARGS>
class WeakCallback
{
public:
	WeakCallback(const std::weak_ptr<CLASS>& object,
					const std::function<void(CLASS*, ARGS...)> function) :
		obj_(object),
		function_(function)
	{
	}

	void operator()(ARGS&&... args) const
	{
		std::shared_ptr<CLASS> ptr(obj_.lock());
		if (ptr)
		{
			function_(ptr.get(), std::forward<ARGS>(args)...);
		}
	}

private:
	std::weak_ptr<CLASS> obj_;
	std::function<void (CLASS*, ARGS... args)> function_;
};

template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(
	const std::shared_ptr<CLASS>& object,
	void(CLASS::* func)(ARGS...))
{
	return WeakCallback<CLASS, ARGS...>(object, func);
	// 不用惊讶 “void(CLASS*::func)(ARGS...) -> void(CLASS*, ARGS...)
	// 因为前者是 class member function，可别忘记 ta 有一个隐藏参数 this :P

	// 展开式
	//return WeakCallback<CLASS, ARGS...>(object,
	//		std::function<void(CLASS*, ARGS...)>(func));
}

template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(
	const std::shared_ptr<CLASS>& obj,
	void (CLASS::* func)(ARGS... args) const)
{
	return WeakCallback<CLASS, ARGS...>(obj, func);
}

}	// namespace Misaka

#endif // !MISAKA_WEAKCALLBACK_H
