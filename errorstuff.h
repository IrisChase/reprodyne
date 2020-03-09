// Copyright 2020 Iris Chase
//
// Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

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
