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
