#pragma once
#include<utility>
#include<typeinfo>
#include<memory>
#include<type_traits>
#include<functional>

template<class _Fnsig>
struct Function
{
	//ֻ��ʹ���˲�����Ret(Args...)ģʽ��FnSigʱ�������ػ������±���
	//�˴����ʽʼ��Ϊfalse,��Ϊ��������ھͱ�������������ģ�����
	static_assert(!std::is_same_v< _Fnsig, _Fnsig>, "not a valid function signature");
	/*
	static_assert �� C++11 �����һ���ǳ����õı���ʱ���Թ��ߡ������������ڱ���׶μ��ĳЩ�����Ƿ�Ϊ�棬������������㣨��Ϊ�٣���������ʧ�ܣ�����ʾָ���Ĵ�����Ϣ��
	������Զ���ģ��Ԫ��̡��ӿ�����Լ�ȷ�������ڱ���ʱ������ĳЩ�ض��������ǳ��а�����
	�����÷�
	static_assert �Ļ����﷨���£�
	static_assert(constant - expression, message);
	constant - expression��������һ������ʱ�������ʽ�������ڱ���ʱ�Ϳ���ȷ�������������ʽ��ֵΪ true���������������Ϊ false�������ʧ�ܡ�
	message����һ����ѡ���ַ�����������������ʧ��ʱ������������ʾ����ַ�����Ϊ������Ϣ��������ṩ��������������һ��Ĭ�ϵĴ�����Ϣ��
	
	std::is_same_v �� C++17 ���������������type trait����һ���֣������ڱ���ʱ������������Ƿ���ͬ��
	std::is_same ��һ����������ģ�壬���̳��� std::integral_constant<bool, B>������ B ��һ������ֵ����ʾ���������Ƿ���ͬ��
	std::is_same_v �� std::is_same ��һ����ݱ�������ͨ�� std::integral_constant �� v ��Ա���� value ��Ա��ֱ���ṩ�˲���ֵ�������������ʽ���ʸó�Ա��
	������˵��std::is_same_v<T, U> �ȼ��� std::is_same<T, U>::value����ǰ�߸��Ӽ���׶���	
	*/
};

template<class _Ret,class ..._Args>
struct Function<_Ret(_Args...)>
{
private:
	struct _FuncBase {
		virtual _Ret _M_call(_Args ...__args) = 0;//���Ͳ������ͳһ�ӿ�
		virtual std::unique_ptr<_FuncBase>_M_clone()const = 0;//ԭ��ģʽ����¡��ǰ��������
		virtual std::type_info const& _M_type() const = 0;//��ú�������������Ϣ
		virtual ~_FuncBase() = default;//Ӧ��_Fn�����з�ƽ�����������
	};
	template<class _Fn>
	struct _FuncImpl :_FuncBase {//FuncImpl�ᱻʵ������Σ�ÿ����ͬ�ķº����඼����һ��ʵ����
		_Fn _M_f;

		template<class ...CArgs>
		explicit _FuncImpl(std::in_place_t,_CArgs&&...__args):_M_f(std::forward(__args)...){}
		/*
		std::in_place_t �� C++17 �����һ�����ͣ�����Ҫ�����ڹ������ʱֱ�������ڴ�λ���Ͻ��о͵ع��죨in-place construction����������ͨ�����ƻ��ƶ���һ������
		�������ͨ������������ std::optional��std::variant��std::map��std::unordered_map �� emplace ������
		�Լ� std::vector �� emplace_back �����ȣ��� emplace �� emplace_back �ȳ�Ա����һ��ʹ�ã���������ܲ����ٲ���Ҫ�ĸ��ƻ��ƶ�������
		std::in_place_t ��һ���յĽṹ�����ͣ�������Я���κ����ݣ�������һ��ռλ�����ǩ����ָʾ���캯�����Ա����Ӧ���������ָ����λ��ֱ�ӹ������
		*/
		_Ret _M_call(_Args ...__args)override {
			//����ת�����в���������ʱ����ķº�������
			//return _M_f(std::forward<_Args>(__args)...);
			//���淶��д����ʵ�ǣ�
			return std::invoke(_M_f, std::forward<_Args>(__args)...);
			/*
			std::invoke �� C++17 �����һ������ģ�壬���ṩ��һ��ͨ�õķ�ʽ�����ÿɵ��ö����纯��������ָ�롢��Ա����ָ�롢��Ա����ָ�롢lambda ���ʽ���󶨱��ʽ�ȣ��Լ���֮������ʵ�Ρ�
			std::invoke ����Ƴ�����Ϊ���ṩһ��ͳһ�Ľӿ���������Щ��ͬ���͵Ŀɵ��ö��󣬴Ӷ��򻯴��벢����ģ��Ԫ��̵���Ҫ��
			�����÷�
			std::invoke �Ļ����﷨���£�
			#include <functional>  
			// ���ÿɵ��ö��� f�������ݲ��� args...  
			auto result = std::invoke(f, args...);
			���У�f �ǿɵ��ö��󣬶� args... �Ǵ��ݸ� f �Ĳ����б�
			*/
		}

		std::unique_ptr<_FuncBase>_M_clone()const override {
			return std::make_unique<_FuncImpl>(_M_f);
		}

		std::type_info const& _M_type() const override {
			return typeid(_Fn);
		}
	};
	std::unique_ptr<_FuncBase>_M_base;//ʹ������ָ�����º�������

public:
	Function() = default;//_M_base ��ʼ��Ϊnullptr
	Function(std::nullptr_t)noexcept:Function(){}

	//�˴�enable_if_t �����ã���ֹFunction�Ӳ��ɵ��õĶ����г�ʼ��
	//�����׼Ҫ��Function����Ҫ�����������֧�ֿ���������_M_clone��
	template <class _Fn, class = std::enable_if_t<std::is_invocable_r_v<_Ret, std::decay_t<_Fn>, _Args...>&& std::is_copy_constructible_v<_Fn>>>
	Function(_Fn&& __f) // û�� explicit������ lambda ���ʽ��ʽת���� Function
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
		//����ת�����в�����������ʹArgs�о������ã�Ҳ�ܲ���������Ŀ�������
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