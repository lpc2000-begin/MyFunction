#pragma once
#include<utility>
#include<typeinfo>
#include<memory>
#include<type_traits>
#include<functional>

template<class _Fnsig>
struct Function
{
	//只在使用了不符合Ret(Args...)模式的FnSig时会进入此特化，导致报错
	//此处表达式始终为false,仅为避免编译期就报错，才让其以来模板参数
	static_assert(!std::is_same_v< _Fnsig, _Fnsig>, "not a valid function signature");
	/*
	static_assert 是 C++11 引入的一个非常有用的编译时断言工具。它允许开发者在编译阶段检查某些条件是否为真，如果条件不满足（即为假），则编译会失败，并显示指定的错误消息。
	这个特性对于模板元编程、接口设计以及确保代码在编译时就满足某些特定的条件非常有帮助。
	基本用法
	static_assert 的基本语法如下：
	static_assert(constant - expression, message);
	constant - expression：必须是一个编译时常量表达式，其结果在编译时就可以确定。如果这个表达式的值为 true，则编译继续；如果为 false，则编译失败。
	message：是一个可选的字符串字面量，当断言失败时，编译器会显示这个字符串作为错误信息。如果不提供，编译器会生成一个默认的错误信息。
	
	std::is_same_v 是 C++17 引入的类型特征（type trait）的一部分，用于在编译时检查两个类型是否相同。
	std::is_same 是一个类型特征模板，它继承自 std::integral_constant<bool, B>，其中 B 是一个布尔值，表示两个类型是否相同。
	std::is_same_v 是 std::is_same 的一个便捷别名，它通过 std::integral_constant 的 v 成员（即 value 成员）直接提供了布尔值结果，而无需显式访问该成员。
	具体来说，std::is_same_v<T, U> 等价于 std::is_same<T, U>::value，但前者更加简洁易读。	
	*/
};

template<class _Ret,class ..._Args>
struct Function<_Ret(_Args...)>
{
private:
	struct _FuncBase {
		virtual _Ret _M_call(_Args ...__args) = 0;//类型擦除后的统一接口
		virtual std::unique_ptr<_FuncBase>_M_clone()const = 0;//原型模式，克隆当前函数对象
		virtual std::type_info const& _M_type() const = 0;//获得函数对象类型信息
		virtual ~_FuncBase() = default;//应对_Fn可能有非平凡析构的情况
	};
	template<class _Fn>
	struct _FuncImpl :_FuncBase {//FuncImpl会被实例化多次，每个不同的仿函数类都产生一次实例化
		_Fn _M_f;

		template<class ...CArgs>
		explicit _FuncImpl(std::in_place_t,_CArgs&&...__args):_M_f(std::forward(__args)...){}
		/*
		std::in_place_t 是 C++17 引入的一个类型，它主要用于在构造对象时直接在其内存位置上进行就地构造（in-place construction），而不是通过复制或移动另一个对象。
		这个类型通常与容器（如 std::optional、std::variant、std::map、std::unordered_map 的 emplace 方法，
		以及 std::vector 的 emplace_back 方法等）的 emplace 或 emplace_back 等成员函数一起使用，以提高性能并减少不必要的复制或移动操作。
		std::in_place_t 是一个空的结构体类型，它本身不携带任何数据，仅用作一个占位符或标签，以指示构造函数或成员函数应该在其参数指定的位置直接构造对象。
		*/
		_Ret _M_call(_Args ...__args)override {
			//完美转发所有参数给构造时保存的仿函数对象
			//return _M_f(std::forward<_Args>(__args)...);
			//更规范的写法其实是：
			return std::invoke(_M_f, std::forward<_Args>(__args)...);
			/*
			std::invoke 是 C++17 引入的一个函数模板，它提供了一种通用的方式来调用可调用对象（如函数、函数指针、成员函数指针、成员数据指针、lambda 表达式、绑定表达式等）以及与之关联的实参。
			std::invoke 的设计初衷是为了提供一种统一的接口来调用这些不同类型的可调用对象，从而简化代码并减少模板元编程的需要。
			基本用法
			std::invoke 的基本语法如下：
			#include <functional>  
			// 调用可调用对象 f，并传递参数 args...  
			auto result = std::invoke(f, args...);
			其中，f 是可调用对象，而 args... 是传递给 f 的参数列表。
			*/
		}

		std::unique_ptr<_FuncBase>_M_clone()const override {
			return std::make_unique<_FuncImpl>(_M_f);
		}

		std::type_info const& _M_type() const override {
			return typeid(_Fn);
		}
	};
	std::unique_ptr<_FuncBase>_M_base;//使用智能指针管理仿函数对象

public:
	Function() = default;//_M_base 初始化为nullptr
	Function(std::nullptr_t)noexcept:Function(){}

	//此处enable_if_t 的作用：阻止Function从不可调用的对象中初始化
	//另外标准要求Function还需要函数对象额外支持拷贝（用于_M_clone）
	template <class _Fn, class = std::enable_if_t<std::is_invocable_r_v<_Ret, std::decay_t<_Fn>, _Args...>&& std::is_copy_constructible_v<_Fn>>>
	Function(_Fn&& __f) // 没有 explicit，允许 lambda 表达式隐式转换成 Function
		: _M_base(std::make_unique<_FuncImpl<std::decay_t<_Fn>>>(std::in_place, std::move(__f)))
	{}
	Function(Function&&) = default;
	Function& operator=(Function&&) = default;

	Function(Function const& __that) :_M_base(__that._M_base ? __that._M_base->clone() : nullptr) {

	}

	Function& operator=(Function const& __that)
	{
		if (__that._M_base)
			_M_base = __that._M_base->clone();
		else
			_M_base = nullptr;
	}

	explicit operator bool()const noexcept {
		return _M_base != nullptr;
	}

	bool operator==(std::nullptr_t)const noexcept {
		return _M_base == nullptr;
	}

	bool operator!=(std::nullptr_t)const noexcept {
		return _M_base != nullptr;
	}

	_Ret operator()(_Args ...__args)const {
		if (!_M_base) [[unlikely]]
			throw std::bad_function_call();
		//完美转发所有参数，这样即使Args中具有引用，也能不产生额外的拷贝开销
		return _M_base->_M_call(std::forward<_Args>(__args)...);
	}

	template<class _Fn>
	_Fn* target()const noexcept {
		return _M_base && typeid(_Fn) == _M_base->_M_type() ? std::addressof(static_cast<_FuncImpl<_Fn>*>(_M_base.get())->_M_f) : nullptr;
	}

	void swap(Function& __that)const noexcept {
		_M_base.swap(__that._M_base);
	}
};