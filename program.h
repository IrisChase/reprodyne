#pragma once

#include <vector>
#include <string>
#include <optional>
#include <map>

#include "user-include/reprodyne.h"
#include "schema_generated.h"

#include "lexcompare.h"

namespace reprodyne
{

class EmptyTape : public std::exception {};

struct Program;

//indeterminate.cpp
double recordIndeterminate(Program* com, void* scope, const char* key, const double val);
double playIndeterminate(Program* com, void* scope, const char* key, const double val);

//callserial.cpp
void recordSerialCall(Program* com, void* scope, const char* key, const char* call);
void validateSerialCall(Program* com, void* scope, const char* key, const char* call);



struct Program
{
    static Reprodyne_playback_failure_handler playbackErrorHandler;


    typedef std::vector<flatbuffers::Offset<reprodyne::IndeterminateEntry>> LiveIndeterminateTape;
    typedef std::vector<flatbuffers::Offset<reprodyne::CallEntry>> LiveCallTape;

    struct LiveVideoTapeEntry
    {
        std::string codec;
        std::vector<unsigned char> data;
        unsigned int width;
        unsigned int height;
    };

    typedef std::vector<LiveVideoTapeEntry> LiveOrdinalVideoTapes;

    struct LiveKeyedScopeEntry
    {
        LiveIndeterminateTape programTape;
        LiveCallTape validationTape;
        LiveOrdinalVideoTapes videoTape;
    };

    typedef std::map<std::string, LiveKeyedScopeEntry> LiveKeyedScopeMap;
    typedef std::vector<LiveKeyedScopeMap> LiveOrdinalScopeTape;

    struct LastReadKey
    {
        int scopeOffset;
        std::string subscopeKey;

        IRISUTILS_DEFINE_COMP(LastReadKey, scopeOffset, subscopeKey)
    };

    struct LastReadVal
    {
        std::optional<int> programPos;
        std::optional<int> callPos;

        //Just the straight frame id, regardless of what we're actually reading.
        //Video tape uses it directly.
        std::optional<int> frameId;
    };


    std::map<void*, int> scopePtrToOrdinalMap;
    std::optional<int> frameCounter;
    std::vector<unsigned char> loadedBuffer;

    std::string jumpSafeString;

    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    LiveOrdinalScopeTape liveTape;
    //TODO: I think a hash would be faster here because there are more reads than writes.
    //TODO: "LookupByKey" doesn't give us an offset into the vector at all, it's a black box.
    //      If this proves to be a bottle kneck, then the only reasonable solution is to perform
    //      a manual binary search on the flatbuffer vector, and store the offset in LastReadKey
    //      instead of "subscopeKey", but in the spirit of no premature optimization...
    std::map<LastReadKey, LastReadVal> lastRead;
    const reprodyne::TapeContainer* coldTape = nullptr;

    void playback_error_handler_wrapper(const int code, const char* msg);
    void error_tape_empty_for_key(const char* prefix, const char* key);

    void logic_error_die(const char* specifically = "Generic logic error.");
    void warning(const char* msg);

    int readScopeOrdinal(void* scopePtr);
    int lastFrameId();

    //Note! the "optionalOffset" parameter is a reference, this prevents it from leaking
    // after a call to playback_error_hanlder_wrapper, it works because the referenced value
    // is actually stored in the "lastRead" structure.
    //Don't get clever and call this with anything else.
    int readOffset(std::optional<int>& optionalOffset, const int tapeSize);

    void assertFrameId(const int frameId, const char* moreSpecifically);
    const reprodyne::KeyedScopeTapeEntry* readKeyedScopeTapeEntry(const int ordinalScopeOffset,
                                                                  const char* subscopeKey,
                                                                  const char* errPrefix);

    //Same across all modes
    void openScope(void* ptr);
    void markFrame();

    //Mode specific, specified in the various classes in modalprogram.h
    virtual void save(const char* path) = 0;

    virtual double intercept_indeterminate(void* scope, const char* key, const double val) = 0;
    virtual void serializeCall(void* scope, const char* key, const char* call) = 0;
};



}//reprodyne
