#pragma once

#include <stdexcept>

namespace reprodyne
{


class PlaybackError : public std::runtime_error
{
    const int code;
    const std::string dynamicErr; //Yeah yeah I know shut up

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
