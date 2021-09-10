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
	// ���þ��� ��void(CLASS*::func)(ARGS...) -> void(CLASS*, ARGS...)
	// ��Ϊǰ���� class member function���ɱ����� ta ��һ�����ز��� this :P

	// չ��ʽ
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
