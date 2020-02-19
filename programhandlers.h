#pragma once

#include "scopecontainers.h"

namespace reprodyne
{

template<typename ScopeContainer>
class Program
{
    std::optional<int> frameCounter;

protected:
    ScopeContainer scopes;

public:
    void openScope(void* ptr)
    { scopes.openScope(ptr); }

    void markFrame()
    {
        if(!frameCounter) frameCounter = 0;
        else ++(*frameCounter);
    }

    int readFrameId()
    {
        if(!frameCounter)
            throw std::logic_error("Call to intercept before reprodyne_mark_frame is forbidden.");
        return *frameCounter;
    }

    template<typename T>
    T intercept(void* scopePtr, const char* subscopeKey, const T indeterminate)
    { return scopes.at(scopePtr).intercept(readFrameId(), subscopeKey, indeterminate); }

    template<typename T>
    void serialize(void* scopePtr, const char* subscopeKey, const T serialValue)
    { scopes.at(scopePtr).serialize(readFrameId(), subscopeKey, serialValue); }

    void assertCompleteRead()
    { scopes.assertCompleteRead(); }
};




class ProgramRecorder : public Program<ScopeContainerRecorder>
{
public:
    void save(const char* path);
};



class ProgramPlayer : public Program<ScopeContainerPlayer>
{
    std::vector<unsigned char> loadedBuffer;

public:
    ProgramPlayer(const char* path);
};




}//reprodyne
