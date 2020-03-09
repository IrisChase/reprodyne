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
