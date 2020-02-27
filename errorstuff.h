#pragma once

#include <stdexcept>

namespace reprodyne
{

class PlaybackError : public std::runtime_error
{
    const int code;

    //Yeah yeah I know about strings and exceptions, and in this case I'll take
    // my chances (This is never thrown on memory error, and memory errors never happen...).
    const std::string dynamicErr;

public:
    PlaybackError(const int code, const std::string dynamicErr):
        code(code),
        dynamicErr(dynamicErr),
        std::runtime_error(dynamicErr.c_str())
    {}

    int getCode() const
    { return code; }
};

}//reprodyne
