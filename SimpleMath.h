#ifndef Simple_MATH_C_H
#define Simple_MATH_C_H

template<typename T> T Cmp(T a, T b){ return a - b; }
template<typename T> bool ApproxEqual(T value, T target, T error){ return abs(value - target) < error; }

#endif