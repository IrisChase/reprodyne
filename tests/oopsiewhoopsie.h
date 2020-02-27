#pragma once

#include <chrono>
#include <exception>

//Common test code, no namespace because it's just for testing and not for
// the library itself.

class OopsieWhoopsie : public std::exception
{
public:
    OopsieWhoopsie(const int code): code(code) {}
    const int code;
};

inline double time()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
