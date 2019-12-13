#include <string>
#include <iostream>

#pragma once

using namespace std;

inline void ERROR(string s) {
  cerr << "Error: " << s << endl;
  exit(1);
}

typedef string Var;

// template <typename T>
// const T min(const T& a, const T& b)
// {
//    return (b < a) ? b : a;
// }
//
// template <typename T>
// const T max(const T& a, const T& b)
// {
//    return (b > a) ? b : a;
// }
