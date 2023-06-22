/**********************************************************************
   NAME: SimpleLoop.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Lock
		Provide loop support
*********************************************************************/

#ifndef Simple_LOOP_C_H
#define Simple_LOOP_C_H

#include <tuple>
#include <utility>

namespace Simple{
    template<int ...>
    struct seq { };

    template<int N, int ...S>
    struct gens : gens<N-1, N-1, S...> { };

    template<int ...S>
    struct gens<0, S...> {
        typedef seq<S...> type;
    };

    template<typename RT, typename ...Args> inline RT apply(RT (*f)(Args...), std::tuple<Args...> t) {
        return apply(f, t, typename gens<sizeof...(Args)>::type());
    }

    template<typename RT, typename ...Args, int ...S>
    inline RT apply(RT (*f)(Args...), std::tuple<Args...> t, seq<S...>) {
        return f(std::get<S>(t)...);
    }
}

#endif