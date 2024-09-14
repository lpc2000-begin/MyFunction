#pragma once
#pragma once

#include <utility>
#include <memory>
#include <type_traits>
#include <functional>

template <class _FnSig>
struct MoveOnlyFunction {
    // ֻ��ʹ���˲����� Ret(Args...) ģʽ�� FnSig ʱ�������ػ������±���
    // �˴����ʽʼ��Ϊ false����Ϊ��������ھͱ�������������ģ�����
    static_assert(!std::is_same_v<_FnSig, _FnSig>, "not a valid function signature");
};

template <class _Ret, class ..._Args>
struct MoveOnlyFunction<_Ret(_Args...)> {
private:
    struct _FuncBase {
        virtual _Ret _M_call(_Args ...__args) = 0; // ���Ͳ������ͳһ�ӿ�
        virtual ~_FuncBase() = default; // Ӧ��_Fn�����з�ƽ�����������
    };

    template <class _Fn>
    struct _FuncImpl : _FuncBase { // FuncImpl �ᱻʵ������Σ�ÿ����ͬ�ķº����඼����һ��ʵ����
        _Fn _M_f;

        template <class ..._CArgs>
        explicit _FuncImpl(std::in_place_t, _CArgs &&...__args) : _M_f(std::forward(__args)...) {}

        _Ret _M_call(_Args ...__args) override {
            // ����ת�����в���������ʱ����ķº�������
            // return _M_f(std::forward<Args>(__args)...);
            // ���淶��д����ʵ�ǣ�
            return std::invoke(_M_f, std::forward<_Args>(__args)...);
        }
    };

    std::unique_ptr<_FuncBase> _M_base; // ʹ������ָ�����º�������

public:
    MoveOnlyFunction() = default; // _M_base ��ʼ��Ϊ nullptr
    MoveOnlyFunction(std::nullptr_t) noexcept : MoveOnlyFunction() {}

    // �˴� enable_if_t �����ã���ֹ MoveOnlyFunction �Ӳ��ɵ��õĶ����г�ʼ��
    // MoveOnlyFunction ��Ҫ��֧�ֿ���
    template <class _Fn, class = std::enable_if_t<std::is_invocable_r_v<_Ret, _Fn&, _Args...>>>
    MoveOnlyFunction(_Fn __f) // û�� explicit������ lambda ���ʽ��ʽת���� MoveOnlyFunction
        : _M_base(std::make_unique<_FuncImpl<_Fn>>(std::in_place, std::move(__f)))
    {}

    // �͵ع���İ汾
    template <class _Fn, class ..._CArgs>
    explicit MoveOnlyFunction(std::in_place_type_t<_Fn>, _CArgs &&...__args)
        : _M_base(std::make_unique<_FuncImpl<_Fn>>(std::in_place, std::forward<_CArgs>(__args)...))
    {}

    MoveOnlyFunction(MoveOnlyFunction&&) = default;
    MoveOnlyFunction& operator=(MoveOnlyFunction&&) = default;
    MoveOnlyFunction(MoveOnlyFunction const&) = delete;
    MoveOnlyFunction& operator=(MoveOnlyFunction const&) = delete;

    explicit operator bool() const noexcept {
        return _M_base != nullptr;
    }

    bool operator==(std::nullptr_t) const noexcept {
        return _M_base == nullptr;
    }

    bool operator!=(std::nullptr_t) const noexcept {
        return _M_base != nullptr;
    }

    _Ret operator()(_Args ...__args) const {
        assert(_M_base);
        // ����ת�����в�����������ʹ Args �о������ã�Ҳ�ܲ���������Ŀ�������
        return _M_base->_M_call(std::forward<_Args>(__args)...);
    }

    void swap(MoveOnlyFunction& __that) const noexcept {
        _M_base.swap(__that._M_base);
    }
};
