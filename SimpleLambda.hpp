/**********************************************************************
   NAME: SimpleIO.h
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Lambda
		Provide functional objects to possibly devices that dont support std::format
		Provides a broadrange of lambda types including heap allocated & local

		I did alot of c++ magic in this file. Congratulations to thy who understand ;)
*********************************************************************/

#ifndef SIMPLE_LAMBDA_C_H
#define SIMPLE_LAMBDA_C_H

#include <tuple>
#include "SimpleLoop.hpp"
#include "SimpleMemory.hpp"

namespace Simple{
    using namespace std;

    template<typename T> struct Template{};
    template<typename TRet, typename ...TArgs> using Function = TRet (*)(TArgs...);

    /**Provide functional objects to possibly devices that dont support std::format
		Provides a broadrange of lambda types including heap allocated & local
     **/
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

    /**Provide functional objects to possibly devices that dont support std::format
      Provides a broadrange of lambda types including heap allocated & local
   **/
    template<typename TFun> struct Lambda : public decltype(__internal_lambda_type__(declval<TFun>())){
        using InternalLambdaType = decltype(__internal_lambda_type__(declval<TFun>()));
        Lambda(){}
        Lambda(void* fn, ref<>& lambda) : InternalLambdaType(fn, lambda){}

        template<typename F>
        static inline Lambda make_lambda(ref<F>& lambda){
            return __make_lambda_internal__<F>(lambda, Template<TFun*>());
        }

    private:

        template<typename F, typename TRet, typename ...TArgs>
        static inline Lambda __make_lambda_internal__(ref<F>& lambda, Template<Function<TRet, TArgs...>>){
            return Lambda((void*) ((Function<TRet, uint8_t*, TArgs...>) [](uint8_t* lam, TArgs... args){ return (*(F*) &*lam)(args...);}), (ref<>&) lambda);
        }
    };

    /**Create a statically allocated lambda.**/
    template<typename TRet, typename ...TArgs> inline Lambda<TRet (TArgs...)> StaticLambda(TRet (*f)(TArgs...)){
        using TF = TRet (*)(TArgs...);
        auto lr = LocalRef(f);
        return Lambda<TRet (TArgs...)>((void*) (TRet (*)(uint8_t*, TArgs...)) ([](uint8_t* lam, TArgs... args){return ((TF) lam)(args...);}), (ref<>&) lr);
    }

    /**Create a locally allocated capturing lambda. WARNING: DO NOT LET THE LAMBDA REFERENCE DIE!!!**/
    template<typename TFun, typename F> inline Lambda<TFun> LocalLambda(F& lambda){
        auto l = LocalRef(&lambda);
        return Lambda<TFun>::make_lambda(l);
    }

    /**Create a globally allocated lambda. Its auto freed when your done :)**/
    template<typename TFun, typename F> inline Lambda<TFun> GlobalLambda(F* lambda){
        auto l = HeapRef(lambda);
        return Lambda<TFun>::make_lambda(l);
    }

    /**Create a globally allocated lambda. Its auto freed when your done :)**/
    template<typename TFun, typename F> inline Lambda<TFun> GlobalLambda(F lambda){ return GlobalLambda<TFun>(&lambda); }

    /**Apply a tuple to a lambda to invoke it**/
    template<typename RT, typename ...Args> inline RT apply(Lambda<RT (Args...)>& l, std::tuple<Args...> t) {
        return apply(l, t, typename gens<sizeof...(Args)>::type());
    }

    /**Apply a tuple to a lambda to invoke it**/
    template<typename RT, typename ...Args, int ...S>
    inline RT apply(Lambda<RT (Args...)>& l, std::tuple<Args...> t, seq<S...>) {
        return l(std::get<S>(t)...);
    }

/**Set the capturing state of the lambda**/
#define capture(...) [__VA_ARGS__]

/**Create a locally allocated capturing lambda
 * Use it like this
 *  //DO NOT LET your_local_name GO OUT OF SCOPE when you invoke the lambda!!!!!!!!!!!
 *  define_local_lambda(your_local_name, [], void, (IOArray& io), io.PrintfEnd("Hello From Timer!"```));
 *  your_local_name(my_io); //Invoke the lamdba here :) **/
#define make_local_lambda(local_name, capture_type, ret, args, ...)               \
        auto CAT(__lambda, __LINE__) = capture_type args -> ret { __VA_ARGS__; };  \
        auto local_name = LocalLambda<ret args>(CAT(__lambda, __LINE__))

/**Create a heap allocated capturing lambda
 * Use it like this
 *  define_global_lambda(your_global_name, [], void, (IOArray& io), io.PrintfEnd("Hello From Timer!"```));
 *  your_global_name(my_io); //Invoke the lamdba here :) **/
#define make_global_lambda(capture_type, ret, args, ...) GlobalLambda<ret args>(capture_type args -> ret { __VA_ARGS__; })
/**Create a statically allocated capturing lambda
 * Use it like this
 * auto my_lam = make_static_lambda(void, (), int i = 2)
 * my_lam();    //Invoke it
 * **/
#define make_static_lambda(ret, args, ...) \
        StaticLambda((ret (*) args) ([] args -> ret { __VA_ARGS__; }))
}
#endif