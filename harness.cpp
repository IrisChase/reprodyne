#include <string>

#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <assert.h>

#include <zlib.h>

#include "schema_generated.h"
#include "user-include/reprodyne.h"

#include "lexcompare.h"

static void reprodyne_default_playback_failure_handler(const int code, const char* msg);

Reprodyne_playback_failure_handler playbackFailureHandler = &reprodyne_default_playback_failure_handler;


std::map<void*, int> scopePtrToOrdinalMap;
std::optional<int> frameCounter;
std::vector<unsigned char> loadedBuffer;

std::string jumpSafeString;

flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();


typedef std::vector<flatbuffers::Offset<reprodyne::IndeterminateEntry>> LiveIndeterminateTape;
typedef std::vector<flatbuffers::Offset<reprodyne::CallEntry>> LiveCallTape;

struct LiveKeyedScopeEntry
{
    LiveIndeterminateTape programTape;
    LiveCallTape validationTape;
};

typedef std::map<std::string, LiveKeyedScopeEntry> LiveKeyedScopeMap;
typedef std::vector<LiveKeyedScopeMap> LiveOrdinalScopeTape;


LiveOrdinalScopeTape liveTape;

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
};

//TODO: I think a hash would be faster here because there are more reads than writes.
//TODO: "LookupByKey" doesn't give us an offset into the vector at all, it's a black box.
//      If this proves to be a bottle kneck, then the only reasonable solution is to perform
//      a manual binary search on the flatbuffer vector, and store the offset in LastReadKey
//      instead of "subscopeKey", but in the spirit of no premature optimization...
std::map<LastReadKey, LastReadVal> lastRead;
const reprodyne::TapeContainer* coldTape = nullptr;

enum class Mode
{
    Record,
    Play,
};
std::optional<Mode> currentMode;


static void reprodyne_default_playback_failure_handler(const int code, const char* msg)
{
    std::cerr << "Reprodyne PLAYBACK FAILURE: " << msg << std::endl << std::flush;
    std::terminate();
}

static void logic_error_die(const std::string specifically = "Generic logic error.")
{
    std::cerr << "Reprodyne INTERNAL ERROR: " << specifically << std::endl << std::flush;
    std::terminate();
}


static int lastFrameId()
{
    if(!frameCounter)
    {
        logic_error_die("Call to intercept before reprodyne_mark_frame() is forbidden.");
    }

    return *frameCounter;
}

static int readOffset(std::optional<int>& optionalOffset, const int tapeSize)
{
    if(!optionalOffset) //Initialize
    {
        if(tapeSize == 0)
        {
            playbackFailureHandler(REPRODYNE_STAT_EMPTY_TAPE, "Tape empty. Bad tape?");
        }

        optionalOffset = 0;
    }

    if(*optionalOffset == tapeSize)
    {
        playbackFailureHandler(REPRODYNE_STAT_TAPE_PAST_END, "Too many reads, tape empty.");
    }

    return (*optionalOffset)++;

}

static void assertFrameId(const int frameId, const char* moreSpecifically)
{
    if(frameId == lastFrameId()) return;

    jumpSafeString = "Frame ID mismatch!\n";
    jumpSafeString += moreSpecifically;
    jumpSafeString += '\n';

    playbackFailureHandler(REPRODYNE_STAT_FRAME_MISMATCH, jumpSafeString.c_str());
}

static std::string readStoredCall(void* scope, const std::string subscopeKey)
{
    const int ordinalScopeOffset = scopePtrToOrdinalMap[scope];

    const reprodyne::KeyedScopeTapeEntry* keyedScope =
            coldTape->ordinalScopeTape()->Get(ordinalScopeOffset)->keyedScopeTape()->LookupByKey(subscopeKey.c_str());

    const int currentTapeOffset =
            readOffset(lastRead[{ordinalScopeOffset, subscopeKey}].callPos, keyedScope->programTape()->size());

    const reprodyne::CallEntry* c = keyedScope->validationTape()->Get(currentTapeOffset);
    
    assertFrameId(c->frameId(), "Serialized calls out of order!");
    
    return c->call()->str();
}

static Mode readMode()
{
    if(!currentMode)
    {
        logic_error_die("Mode not set! Initialize Harness with play(x3th_path) or record()!");
    }

    return *currentMode;
}

static void init(const Mode theMode)
{
    reprodyne_do_not_call_this_function_directly_reset();
    currentMode = theMode;
}

extern "C"
{

void reprodyne_do_not_call_this_function_directly_reset()
{
    scopePtrToOrdinalMap.clear();
    frameCounter.reset();
    jumpSafeString.clear();

    coldTape = nullptr; //Looks like it's just a fancy shmancy reinterpret_cast'd pointer into our buffer
    loadedBuffer.clear();
    builder.Clear();

    liveTape.clear();
    lastRead.clear();

    currentMode.reset();
}

void reprodyne_do_not_call_this_function_directly_set_playback_failure_handler(Reprodyne_playback_failure_handler handler)
{
    playbackFailureHandler = handler; //Not my problem anymore...
}

void reprodyne_do_not_call_this_function_directly_record()
{ init(Mode::Record); }


void reprodyne_do_not_call_this_function_directly_save(const char* path)
{
    if(readMode() == Mode::Play)
    {
        std::cerr << "Reprodyne ERROR: Cannot save results in Playback mode!" << std::endl;
        return;
    }


    std::ofstream file(path, std::ios_base::binary);

    std::vector<flatbuffers::Offset<reprodyne::OrdinalScopeTapeEntry>> ordinalScopeTape;

    for(LiveKeyedScopeMap& subScopes : liveTape)
    {
        std::vector<flatbuffers::Offset<reprodyne::KeyedScopeTapeEntry>> keyedScopeTape;

        for(auto pair : subScopes)
        {
            const std::string key = pair.first;
            const LiveIndeterminateTape& programTape = pair.second.programTape;
            const LiveCallTape& callTape = pair.second.validationTape;
            

            const auto fbString = builder.CreateString(key);

            const auto fbProgramTape = builder.CreateVector(programTape);
            const auto fbCallTape = builder.CreateVector(callTape);

            const auto fbKeyedScopeEntry = reprodyne::CreateKeyedScopeTapeEntry(builder,
                                                                               fbString,
                                                                               fbProgramTape,
                                                                               fbCallTape);

            keyedScopeTape.push_back(fbKeyedScopeEntry);
        }

        const auto fbKeyedScopeTape = builder.CreateVectorOfSortedTables(&keyedScopeTape);

        const auto fbOrdinalScopeTapeEntry = reprodyne::CreateOrdinalScopeTapeEntry(builder, fbKeyedScopeTape);

        ordinalScopeTape.push_back(fbOrdinalScopeTapeEntry);
    }

    const auto fbOrdinalScopeTape = builder.CreateVector(ordinalScopeTape);
    const auto fbTapeContainer = reprodyne::CreateTapeContainer(builder, fbOrdinalScopeTape);

    builder.Finish(fbTapeContainer);

    file.write(reinterpret_cast<char*>(builder.GetBufferPointer()), builder.GetSize());
}

void reprodyne_do_not_call_this_function_directly_play(const char* path)
{
    init(Mode::Play);

    std::ifstream file(path, std::ios_base::binary);

    loadedBuffer = std::vector<unsigned char>(std::istreambuf_iterator(file), {});

    coldTape = reprodyne::GetTapeContainer(&loadedBuffer[0]);
}

void reprodyne_do_not_call_this_function_directly_open_scope(void* ptr)
{
    liveTape.emplace_back();
    scopePtrToOrdinalMap[ptr] = liveTape.size() - 1;
}

void reprodyne_do_not_call_this_function_directly_mark_frame()
{
    if(!frameCounter) frameCounter = 0;
    else ++(*frameCounter);
}

void reprodyne_do_not_call_this_function_directly_write_indeterminate(void* scope, const char* key, double indeterminate)
{
    if(!(readMode() == Mode::Record))
    {
        std::cerr << "Reprodyne WARNING: write_indeterminate called in non-record mode!" << std::endl;
        return;
    }

    auto indeterminateOffset = reprodyne::CreateIndeterminateEntry(builder, lastFrameId(), indeterminate);
    liveTape[scopePtrToOrdinalMap[scope]][key].programTape.push_back(indeterminateOffset);
}

double reprodyne_do_not_call_this_function_directly_read_indeterminate(void* scope, const char* subscopeKey)
{
    const int ordinalScopeOffset = scopePtrToOrdinalMap[scope];

    //I just don't feel like naming all the intermediates, bite me.
    const reprodyne::KeyedScopeTapeEntry* keyedScope =
            coldTape->ordinalScopeTape()->Get(ordinalScopeOffset)->keyedScopeTape()->LookupByKey(subscopeKey);

    const int currentTapeOffset =
            readOffset(lastRead[{ordinalScopeOffset, subscopeKey}].programPos, keyedScope->programTape()->size());

    const reprodyne::IndeterminateEntry* i = keyedScope->programTape()->Get(currentTapeOffset);

    assertFrameId(i->frameId(), "Determinates out of order!");

    return i->val();
}

double reprodyne_do_not_call_this_function_directly_intercept_indeterminate(void* scopePtr, const char* scopeKey, const double indeterminate)
{
    if(readMode() == Mode::Record)
    {
        reprodyne_do_not_call_this_function_directly_write_indeterminate(scopePtr, scopeKey, indeterminate);
        return indeterminate;
    }
    else if(readMode() == Mode::Play)
    {
        return reprodyne_do_not_call_this_function_directly_read_indeterminate(scopePtr, scopeKey);
    }

    logic_error_die();
}

void reprodyne_do_not_call_this_function_directly_serialize(void* scope, const char* subScopeKey, const char* call)
{
    if(readMode() == Mode::Record)
    {
        const std::string cppStringCall = call;
        auto stringOffset = builder.CreateString(cppStringCall.c_str(), cppStringCall.size());
        auto callEntryOffset = reprodyne::CreateCallEntry(builder, lastFrameId(), stringOffset);
        
        liveTape[scopePtrToOrdinalMap[scope]][subScopeKey].validationTape.push_back(callEntryOffset);
    }
    else if(readMode() == Mode::Play)
    {
        bool failure = false;

        //This is convoluted because we need to ensure that the destructor for
        // "storedCall" is invoked before we call the playbackFailureHandler,
        // which may longjmp back out of here.
        {
            const std::string storedCall = readStoredCall(scope, subScopeKey);

            if(storedCall != call)
            {
                failure = true;
                jumpSafeString = "Stored call mismatch! Program produced: \n";
                jumpSafeString += call;
                jumpSafeString += "Was expecting: \n";
                jumpSafeString += storedCall;
                jumpSafeString += '\n';
            }
        }

        if(failure) playbackFailureHandler(REPRODYNE_STAT_CALL_MISMATCH, jumpSafeString.c_str());
    }
    else logic_error_die("Mode corrupt or not set somehow.");
}

} //extern "C"
