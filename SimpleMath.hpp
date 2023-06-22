/**********************************************************************
   NAME: SimpleMath.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Lock
		Provide math support
*********************************************************************/

#ifndef Simple_MATH_C_H
#define Simple_MATH_C_H

template<typename T> T Cmp(T a, T b){ return a - b; }
template<typename T> bool ApproxEqual(T value, T target, T error){ return abs(value - target) < error; }

#endif