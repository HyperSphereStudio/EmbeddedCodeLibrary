#ifndef SIMPLE_LAMBDA_C_H
#define SIMPLE_LAMBDA_C_H

#include <tuple>
#include "SimpleLoop.h"
#include "SimpleMemory.h"

namespace Simple{
    using namespace std;

    template<typename T> struct Template{};
    template<typename TRet, typename ...TArgs> using Function = TRet (*)(TArgs...);

    template<typename TReturn, typename ...TArgs>
    struct InternalLambda{
        using ReturnType = TReturn;
        using Args = tuple<TArgs...>;
        using UnderlyingFunctionType = Function<TReturn, uint8_t*, TArgs...>;
        using ExposedFunctionType = Function<TReturn, TArgs...>;
    private:
        UnderlyingFunctionType fn = nullptr;
        ref<> captured_lambda;
    protected:
        InternalLambda(){}
        InternalLambda(void* fn, ref<>& lambda) : fn((UnderlyingFunctionType) fn), captured_lambda(lambda){}
    public:
        inline TReturn operator ()(TArgs... args){ return fn(captured_lambda.get(), args...); }
    };

    template<typename TRet, typename ...TArgs>
    inline static InternalLambda<TRet, TArgs...> __internal_lambda_type__(TRet (*)(TArgs...)){ return InternalLambda<TRet, TArgs...>(); };

    template<typename TFun> struct Lambda : public decltype(__internal_lambda_type__(declval<TFun>())){
        using InternalLambdaType = decltype(__internal_lambda_type__(declval<TFun>()));
        Lambda(){}

        template<typename F>
        static inline Lambda make_lambda(ref<F>& lambda){
            return __make_lambda_internal__<F>(lambda, Template<TFun*>());
        }

    private:
        Lambda(void* fn, ref<>& lambda) : InternalLambdaType(fn, lambda){}

        template<typename F, typename TRet, typename ...TArgs>
        static inline Lambda __make_lambda_internal__(ref<F>& lambda, Template<Function<TRet, TArgs...>>){
            return Lambda((void*) ((Function<TRet, uint8_t*, TArgs...>) [](uint8_t* lam, TArgs... args){ return (*(F*) &*lam)(args...);}), (ref<>&) lambda);
        }
    };

    template<typename TRet, typename ...TArgs> Lambda<TRet (TArgs...)> GlobalLambda(TRet (*f)(TArgs...)){
        using TF = TRet (*)(TArgs...);
        auto lr = LocalRef(f);
        return Lambda<TF>((void*) ((TF) [](uint8_t* lam, TArgs... args){return ((TF) lam)(args...);}), lr);
    }
    template<typename TFun, typename F> inline Lambda<TFun> LocalLambda(F& lambda){
        auto l = LocalRef(&lambda);
        return Lambda<TFun>::make_lambda(l);
    }
    template<typename TFun, typename F> inline Lambda<TFun> GlobalLambda(F* lambda){
        auto l = HeapRef(lambda);
        return Lambda<TFun>::make_lambda(l);
    }
    template<typename TFun, typename F> inline Lambda<TFun> GlobalLambda(F lambda){ return GlobalLambda<TFun>(&lambda); }

    template<typename RT, typename ...Args> inline RT apply(Lambda<RT (Args...)>& l, std::tuple<Args...> t) {
        return apply(l, t, typename gens<sizeof...(Args)>::type());
    }

    template<typename RT, typename ...Args, int ...S>
    inline RT apply(Lambda<RT (Args...)>& l, std::tuple<Args...> t, seq<S...>) {
        return l(std::get<S>(t)...);
    }

#define capture(...) [__VA_ARGS__]
#define define_local_lambda(local_name, capture_type, ret, args, ...)               \
        auto CAT(__lambda, __LINE__) = capture_type args -> ret { __VA_ARGS__; };  \
        auto local_name = LocalLambda<ret args>(CAT(__lambda, __LINE__))

#define define_global_lambda(local_name, capture_type, ret, args, ...) \
        auto CAT(__lambda, __LINE__) = capture_type args -> ret { __VA_ARGS__; };  \
        auto local_name = GlobalLambda<ret args>(CAT(__lambda, __LINE__))
}
#endif