#include <string>
#include <iostream>

#pragma once

using namespace std;

inline void ERROR(string s) {
  cerr << "Error: " << s << endl;
  exit(1);
}

typedef string Var;
