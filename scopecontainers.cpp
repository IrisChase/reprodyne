#include "scopecontainers.h"

#include "user-include/reprodyne.h"

namespace reprodyne
{

ScopeHandlerRecorder& ScopeContainerRecorder::at(void* ptr)
{
    auto ordinalIterator = ordinalMap.find(ptr);
    if(ordinalIterator == ordinalMap.end())
        throw PlaybackError(REPRODYNE_STAT_UNREGISTERED_SCOPE, "Unregistered scope!");

    return storedScope.at(ordinalIterator->second);
}




void ScopeContainerPlayer::openScope(void* ptr)
{
    if(ordinalPosition == myRootBuffer->end()) throw std::runtime_error("Ordinal scope buffer overread");

    auto it = scopeMap.find(ptr);

    if(it != scopeMap.end())
    {
        try
        {
            it->second.assertCompletReed();
        }
        catch(const PlaybackError& e)
        {
            //Current interface doesn't expect such eager playback errors, so we stash it for now~
            deferredCompleteReadErrors.push_back(e);
        }
    }

    scopeMap.insert_or_assign(ptr, ScopeHandlerPlayer(*ordinalPosition));
    ++ordinalPosition;
}

ScopeHandlerPlayer& ScopeContainerPlayer::at(void* ptr)
{
    auto it = scopeMap.find(ptr);
    if(it == scopeMap.end()) throw PlaybackError(REPRODYNE_STAT_UNREGISTERED_SCOPE, "Unregistered scope!");
    return it->second;
}

}//reprodyne
