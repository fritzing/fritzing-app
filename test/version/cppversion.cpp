#include <iostream>

/*
  A quick compile test to verify that certain C++ features are supported
 */

template<typename T>
constexpr T pi = T(3.1415926535897932385);

void testCppVersion() {
    int i = 1;
    int& r = i;
    auto ar = r;                                // int, nicht: int&
    decltype(r) dr = r;                         // int& C++11/14
    decltype(auto) dra = r;                     // int& C++14


    std::cout << 0b0001'0000'0001;
}