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

enum class Mode
{
    Record,
    Play,
};

template<typename IndeterminateType>
class Recorder
{
    std::vector<IndeterminateType> tape;

public:
    IndeterminateType record(const IndeterminateType indeterminate);
};

template<typename IndeterminateType>
class Player
{
public:
    IndeterminateType readNext();
};

class ScopeRecorder
{
    //Key is subscope key
    std::map<std::string, Recorder<double>> theDubbles;
    std::map<std::string, Recorder<int>> theInts;

public:
    double react(const char* subScopeKey, const double indeterminate)
    { return theDubbles[subScopeKey].record(indeterminate); }

    int react(const char* subScopeKey, const int indeterminate)
    { return theInts[subScopeKey].record(indeterminate); }
};

class ScopePlayer
{
public:
    double react(const char* subScopeKey, const double indeterminate);
    int react(const char* subScopeKey, const int indeterminate);
};

template<typename T>
class ScopeContainer
{
    std::vector<T> storedScope;
    std::map<void*, int> ordinalMap;

public:
    void openScope(void* ptr)
    {
        storedScope.emplace_back();
        ordinalMap[ptr] = storedScope.size() - 1;
    }

    T& at(void* ptr)
    {
        auto ordinalIterator = ordinalMap.find(ptr);
        if(ordinalIterator == ordinalMap.end())
        {
            //todo: throw scope out of range
            throw std::runtime_error("FIXME");
        }

        return storedScope.at(ordinalIterator->second);
    }

    std::vector<T> pop()
    {
        ordinalMap.clear();
        return std::move(storedScope);
    }
};

template<typename ScopeHandler>
class Program
{
protected:
    ScopeContainer<ScopeHandler> scopes;

public:
    void openScope(void* ptr)
    { scopes.openScope(ptr); }

    template<typename T>
    T intercept(void* scopePtr, const char* subscopeKey, const T indeterminate)
    { return scopes.at(scopePtr).react(subscopeKey, indeterminate); }
};

class ProgramRecorder : public Program<ScopeRecorder>
{
public:
    void saveEtc(); //Accessing scope
};

class ProgramPlayer : public Program<ScopePlayer>
{
public:
    void loadEtc();
};



class State
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

    std::optional<Mode> currentMode;

public:
    static void setPlaybackErrorHandler(Reprodyne_playback_failure_handler handler)
    { playbackErrorHandler = handler; }

    State() {}
    State(const Mode mode): currentMode(mode) {}

    Mode readMode();

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

    void writeIndeterminate(void* scopePtr, const char* key, const double indeterminate);
    double readIndeterminate(void* scopePtr, const char* key);

    void serializeCall(void* scopePtr, const char* subScopeKey, const char* call);
    std::string readStoredCall(void* scope, const char* subscopeKey);


    void openScope(void* ptr);
    void markFrame();
    void save(const char* path);
    void load(const char* path);
    void assertCompleteRead();
};



}//reprodyne
