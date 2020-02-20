#pragma once

#include "scopehandlers.h"

#include "errorstuff.h"

namespace reprodyne
{

class ScopeContainerRecorder
{
    flatbuffers::FlatBufferBuilder& builder;

    std::vector<ScopeHandlerRecorder> storedScope;
    std::map<void*, int> ordinalMap;

public:
    ScopeContainerRecorder(flatbuffers::FlatBufferBuilder& builder): builder(builder) {}

    void openScope(void* ptr)
    {
        storedScope.emplace_back(builder);
        ordinalMap[ptr] = storedScope.size() - 1;
    }

    ScopeHandlerRecorder& at(void* ptr);

    std::vector<ScopeHandlerRecorder> pop()
    {
        ordinalMap.clear();
        return std::move(storedScope);
    }
};




class ScopeContainerPlayer
{
public:
    typedef flatbuffers::Vector<flatbuffers::Offset<OrdinalScopeTapeEntry>> BufferType;

private:
    const BufferType* myRootBuffer;
    BufferType::const_iterator ordinalPosition;

    std::map<void*, ScopeHandlerPlayer> scopeMap;

    std::vector<PlaybackError> deferredCompleteReadErrors;

public:
    void openScope(void *ptr);

    ScopeHandlerPlayer& at(void* ptr);

    void load(const BufferType* rootBeer)
    {
        myRootBuffer = rootBeer;
        ordinalPosition = myRootBuffer->begin();
    }

    void assertCompleteRead()
    {
        for(auto& e : deferredCompleteReadErrors) throw e; //Only the first one but that's cool too.
        for(auto pair : scopeMap) pair.second.assertCompletReed();
    }
};


}//reprodyne
