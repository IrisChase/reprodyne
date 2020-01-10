#include "program.h"


#include <iostream>

namespace reprodyne
{

void reprodyne_default_playback_failure_handler(const int, const char* msg)
{
    std::cerr << "Reprodyne PLAYBACK FAILURE: " << msg << std::endl << std::flush;
    std::terminate();
}
Reprodyne_playback_failure_handler Program::playbackErrorHandler =
        &reprodyne_default_playback_failure_handler;


void Program::playback_error_handler_wrapper(const int code, const char* msg)
{
    playbackErrorHandler(code, msg);
    std::terminate(); //If you don't do it I will
}

void Program::error_tape_empty_for_key(const char* prefix, const char* key)
{
    jumpSafeString = prefix;
    jumpSafeString += " tape empty for subscope key: ";
    jumpSafeString += key;
    jumpSafeString.push_back('\n');
    playback_error_handler_wrapper(REPRODYNE_STAT_EMPTY_TAPE, jumpSafeString.c_str());
}

void Program::logic_error_die(const char* specifically)
{
    std::cerr << "Reprodyne INTERNAL ERROR: " << specifically << std::endl << std::flush;
    std::terminate();
}

void Program::warning(const char* msg)
{
    std::cerr << "Reprodyne WARNING: " << msg << std::endl;
}

int Program::readScopeOrdinal(void* scopePtr)
{
    auto it = scopePtrToOrdinalMap.find(scopePtr);
    if(it == scopePtrToOrdinalMap.end())
        playback_error_handler_wrapper(REPRODYNE_STAT_UNREGISTERED_SCOPE, "Scope ptr unrecognized.");

    return it->second;
}

int Program::lastFrameId()
{
    if(!frameCounter)
    {
        logic_error_die("Call to intercept before reprodyne_mark_frame() is forbidden.");
    }

    return *frameCounter;
}

int Program::readOffset(std::optional<int>& optionalOffset, const int tapeSize)
{
    if(!optionalOffset) //Initialize
    {
        if(tapeSize == 0)
        {
            playback_error_handler_wrapper(REPRODYNE_STAT_EMPTY_TAPE, "Tape empty. Bad tape?");
        }

        optionalOffset = 0;
    }

    if(*optionalOffset == tapeSize)
    {
        playback_error_handler_wrapper(REPRODYNE_STAT_TAPE_PAST_END, "Too many reads, tape empty.");
    }

    return (*optionalOffset)++;
}

void Program::assertFrameId(const int frameId, const char* moreSpecifically)
{
    if(frameId == lastFrameId()) return;

    jumpSafeString = "Frame ID mismatch!\n";
    jumpSafeString += moreSpecifically;
    jumpSafeString += '\n';

    playback_error_handler_wrapper(REPRODYNE_STAT_FRAME_MISMATCH, jumpSafeString.c_str());
}

const KeyedScopeTapeEntry* Program::readKeyedScopeTapeEntry(const int ordinalScopeOffset, const char* subscopeKey, const char* errPrefix)
{
    try
    {
        //We assume ordinalScopeOffset is safe.
        auto keyedScopeTape = coldTape->ordinalScopeTape()->Get(ordinalScopeOffset)->keyedScopeTape();
        if(!keyedScopeTape) throw reprodyne::EmptyTape();

        auto tapeEntry = keyedScopeTape->LookupByKey(subscopeKey);
        if(!tapeEntry) throw reprodyne::EmptyTape();

        return tapeEntry;
    }
    catch(const reprodyne::EmptyTape)
    {
        error_tape_empty_for_key(errPrefix, subscopeKey);
    }
}



}//reprodyne
