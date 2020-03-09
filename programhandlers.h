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

#include "scopecontainers.h"

namespace reprodyne
{

template<typename ScopeContainer>
class Program
{
    std::optional<unsigned int> frameCounter;

protected:
    ScopeContainer scopes;

public:
    Program() {}
    Program(ScopeContainer cont): scopes(cont) {}

    void openScope(void* ptr)
    { scopes.openScope(ptr); }

    void markFrame()
    {
        if(!frameCounter) frameCounter = 0;
        else ++(*frameCounter);
    }

    unsigned int readFrameId()
    {
        if(!frameCounter)
            throw std::logic_error("Call to intercept before reprodyne_mark_frame is forbidden.");
        return *frameCounter;
    }

    template<typename T>
    T intercept(void* scopePtr, const char* subscopeKey, const T indeterminate)
    { return scopes.at(scopePtr).intercept(readFrameId(), subscopeKey, indeterminate); }

    template<typename... Ts>
    void serialize(void* scopePtr, const char* subscopeKey, const Ts... serialValue)
    { scopes.at(scopePtr).serialize(readFrameId(), subscopeKey, serialValue...); }

    void assertCompleteRead()
    { scopes.assertCompleteRead(); }
};




class ProgramRecorder : public Program<ScopeContainerRecorder>
{
    flatbuffers::FlatBufferBuilder builder;

public:
    ProgramRecorder(): Program(ScopeContainerRecorder(builder)) {}

    void save(const char* path);
};



class ProgramPlayer : public Program<ScopeContainerPlayer>
{
    std::vector<unsigned char> loadedBuffer;

public:
    ProgramPlayer(const char* path);
};




}//reprodyne
