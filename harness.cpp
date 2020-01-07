#include <string>

#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <assert.h>

#include <zlib.h>

#include <libavcodec/avcodec.h>

#include "schema_generated.h"
#include "user-include/reprodyne.h"
#include "fileformat.h"
#include "liverecording.h"

#include "lexcompare.h"

static void reprodyne_default_playback_failure_handler(const int code, const char* msg);

Reprodyne_playback_failure_handler playbackErrorHandler = &reprodyne_default_playback_failure_handler;

namespace reprodyne
{

class EmptyTape : public std::exception {};

}



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

static void playback_error_handler_wrapper(const int code, const char* msg)
{
    playbackErrorHandler(code, msg);
    std::terminate(); //If you do it I will.
}

static void error_tape_empty_for_key(const char* prefix, const char* key)
{
    jumpSafeString = prefix;
    jumpSafeString += " tape empty for subscope key: ";
    jumpSafeString += key;
    jumpSafeString.push_back('\n');
    playback_error_handler_wrapper(REPRODYNE_STAT_EMPTY_TAPE, jumpSafeString.c_str());
}

static void logic_error_die(const char* specifically = "Generic logic error.")
{
    std::cerr << "Reprodyne INTERNAL ERROR: " << specifically << std::endl << std::flush;
    std::terminate();
}

static void warning(const char* msg)
{
    std::cerr << "Reprodyne WARNING: " << msg << std::endl;
}

static int readScopeOrdinal(void* scopePtr)
{
    auto it = scopePtrToOrdinalMap.find(scopePtr);
    if(it == scopePtrToOrdinalMap.end())
        playback_error_handler_wrapper(REPRODYNE_STAT_UNREGISTERED_SCOPE, "Scope ptr unrecognized.");

    return it->second;
}

static int lastFrameId()
{
    if(!frameCounter)
    {
        logic_error_die("Call to intercept before reprodyne_mark_frame() is forbidden.");
    }

    return *frameCounter;
}

//Note! the "optionalOffset" parameter is a reference, this prevents it from leaking
// after a call to playback_error_hanlder_wrapper, it works because the reference is actually
// stored in the "lastRead" structure. Don't get clever and call this with anything else.
static int readOffset(std::optional<int>& optionalOffset, const int tapeSize)
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

static void assertFrameId(const int frameId, const char* moreSpecifically)
{
    if(frameId == lastFrameId()) return;

    jumpSafeString = "Frame ID mismatch!\n";
    jumpSafeString += moreSpecifically;
    jumpSafeString += '\n';

    playback_error_handler_wrapper(REPRODYNE_STAT_FRAME_MISMATCH, jumpSafeString.c_str());
}

static const reprodyne::KeyedScopeTapeEntry* readKeyedScopeTapeEntry(const int ordinalScopeOffset,
                                                                     const char* subscopeKey,
                                                                     const char* errPrefix)
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

static std::string readStoredCall(void* scopePtr, const char* subscopeKey)
{
    const int ordinalScopeOffset = readScopeOrdinal(scopePtr);
    const auto validationTape = readKeyedScopeTapeEntry(ordinalScopeOffset, subscopeKey, "Call")->validationTape();

    if(!validationTape)
    {
        jumpSafeString = "Validation tape empty for key: ";
        jumpSafeString += subscopeKey;
        jumpSafeString.push_back('\n');
        playback_error_handler_wrapper(REPRODYNE_STAT_EMPTY_TAPE, jumpSafeString.c_str());
    }

    const reprodyne::CallEntry* c =
            validationTape->Get(readOffset(lastRead[{ordinalScopeOffset, subscopeKey}].callPos, validationTape->size()));

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
    playbackErrorHandler = handler; //Not my problem anymore...
}

void reprodyne_do_not_call_this_function_directly_record()
{ init(Mode::Record); }


void reprodyne_do_not_call_this_function_directly_save(const char* path)
{
    if(readMode() == Mode::Play)
    {
        std::cerr << "Reprodyne WARNING: Cannot save results in Playback mode!" << std::endl;
        return;
    }



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

    //0 because I wanna make sure the upper bits are zeroed.
    uint64_t compressionRegionSize = 0 + compressBound(builder.GetSize());
    std::vector<Bytef> outputBuffer(reprodyne::FileFormat::reservedRangeSize + compressionRegionSize);

    reprodyne::FileFormat::writeBoringStuffToReservedRegion(&outputBuffer[0], compressionRegionSize);

    if(compress(&outputBuffer[reprodyne::FileFormat::compressedDataRegionOffset],
                &compressionRegionSize,
                builder.GetBufferPointer(),
                builder.GetSize())
            != Z_OK)
    {
        logic_error_die("Some problem with zlib");
    }

    //compressionRegionSize is updated by compress
    const int finalFileSize = reprodyne::FileFormat::reservedRangeSize + compressionRegionSize;
    std::ofstream(path, std::ios_base::binary).write(reinterpret_cast<char*>(&outputBuffer[0]), finalFileSize);
}

void reprodyne_do_not_call_this_function_directly_play(const char* path)
{
    init(Mode::Play);

    {
        std::ifstream file(path, std::ios_base::binary);
        auto tempBuffer = std::vector<unsigned char>(std::istreambuf_iterator(file), {});

        if(tempBuffer.size() < reprodyne::FileFormat::reservedRangeSize)
            logic_error_die("Bad file, too small to even hold the reserved region...");

        if(!reprodyne::FileFormat::checkSignature(&tempBuffer[0]))
            logic_error_die("Unrecognized file signature.");

        if(!reprodyne::FileFormat::checkVersion(&tempBuffer[0]))
            logic_error_die("File is not compatible with this version of reprodyne.");

        uint64_t destBuffSize = reprodyne::FileFormat::readUncompressedSize(&tempBuffer[0]);
        loadedBuffer = std::vector<unsigned char>(destBuffSize);

        //READ MOTHERGUCGKer
        const int compressedRegionSize = tempBuffer.size() - reprodyne::FileFormat::reservedRangeSize;
        const auto stat =
                uncompress(&loadedBuffer[0], &destBuffSize,
                           &tempBuffer[reprodyne::FileFormat::compressedDataRegionOffset], compressedRegionSize);

        if(stat == Z_MEM_ERROR) logic_error_die("Not enough memory for zlib.");
        if(stat == Z_BUF_ERROR) logic_error_die("Data corrupt or incomplete.");
        if(stat != Z_OK) logic_error_die("Some unknown error occurred during decompression.");
    }

    coldTape = reprodyne::GetTapeContainer(&loadedBuffer[0]);
}

void reprodyne_do_not_call_this_function_directly_assert_complete_read()
{
    if(readMode() != Mode::Play)
    {
        warning("Tape assert was called in a mode other than playback. This is ill-formed.");
        return;
    }

    if(!coldTape->ordinalScopeTape()) return;

    for(int ordinalScope = 0; ordinalScope != coldTape->ordinalScopeTape()->size(); ++ordinalScope)
    {
        if(!coldTape->ordinalScopeTape()->Get(ordinalScope)->keyedScopeTape()) continue;

        for(auto keyedEntry : *coldTape->ordinalScopeTape()->Get(ordinalScope)->keyedScopeTape())
        {
            //These have non-trivial destructors, so we have to ensure that they fall out of scope before
            // failing. TODO: This might change, if LastReadKey gets an integer instead of a string
            // for it's key value.
            const LastReadKey readKey = {ordinalScope, keyedEntry->key()->str()};
            const LastReadVal readVal = lastRead[readKey];

            auto setErrString = [&](const std::string type)
            {
                jumpSafeString = type;
                jumpSafeString += " tape was not read to completion for scope key: ";
                jumpSafeString += readKey.subscopeKey;
                jumpSafeString.push_back('\n');
            };

            auto compareReadPosToSize = [&](const auto& readVal, const int size)
            {
                return readVal ? *readVal == size : !size;
            };

            if(!compareReadPosToSize(readVal.programPos, keyedEntry->programTape()->size()))
            {
                setErrString("Program");
                goto progTapeFailure;
            }
            if(!compareReadPosToSize(readVal.callPos, keyedEntry->validationTape()->size()))
            {
                setErrString("Call");
                goto validationTapeFailure;
            }
        }
    }

    return;
    progTapeFailure: playback_error_handler_wrapper(REPRODYNE_STAT_PROG_TAPE_INCOMPLETE_READ, jumpSafeString.c_str());
    validationTapeFailure: playback_error_handler_wrapper(REPRODYNE_STAT_CALL_TAPE_INCOMPLETE_READ, jumpSafeString.c_str());
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

void reprodyne_do_not_call_this_function_directly_write_indeterminate(void* scopePtr, const char* key, double indeterminate)
{
    if(readMode() == Mode::Play)
    {
        std::cerr << "Reprodyne WARNING: write_indeterminate called in non-record mode!" << std::endl;
        return;
    }

    auto indeterminateOffset = reprodyne::CreateIndeterminateEntry(builder, lastFrameId(), indeterminate);
    liveTape[readScopeOrdinal(scopePtr)][key].programTape.push_back(indeterminateOffset);
}

double reprodyne_do_not_call_this_function_directly_read_indeterminate(void* scopePtr, const char* subscopeKey)
{
    const int ordinalScopeOffset = readScopeOrdinal(scopePtr);

    //I just don't feel like naming all the intermediates, bite me.
    const auto programTape = readKeyedScopeTapeEntry(ordinalScopeOffset, subscopeKey, "Program")->programTape();

    if(!programTape)
    {
        error_tape_empty_for_key("Program", subscopeKey);
    }

    const reprodyne::IndeterminateEntry* i =
            programTape->Get(readOffset(lastRead[{ordinalScopeOffset, subscopeKey}].programPos,
                                           programTape->size()));

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

void reprodyne_do_not_call_this_function_directly_serialize(void* scopePtr, const char* subScopeKey, const char* call)
{
    const int scopeOrdinal = readScopeOrdinal(scopePtr);
    if(readMode() == Mode::Record)
    {
        const std::string cppStringCall = call;
        auto stringOffset = builder.CreateString(cppStringCall.c_str(), cppStringCall.size());
        auto callEntryOffset = reprodyne::CreateCallEntry(builder, lastFrameId(), stringOffset);
        
        liveTape[scopeOrdinal][subScopeKey].validationTape.push_back(callEntryOffset);
    }
    else if(readMode() == Mode::Play)
    {
        bool failure = false;

        //This is convoluted because we need to ensure that the destructor for
        // "storedCall" is invoked before we call the playback_error_handler_wrapper,
        // which may longjmp back out of here.
        {
            //readStoredCall can jump
            jumpSafeString = readStoredCall(scopePtr, subScopeKey);

            if(jumpSafeString != call)
            {
                failure = true;

                const std::string storedCall = jumpSafeString;
                jumpSafeString = "Stored call mismatch! Program produced: \n";
                jumpSafeString += call;
                jumpSafeString += "Was expecting: \n";
                jumpSafeString += storedCall;
                jumpSafeString += '\n';
            }

            if(failure) playback_error_handler_wrapper(REPRODYNE_STAT_CALL_MISMATCH, jumpSafeString.c_str());
        }
    }
    else logic_error_die("Mode corrupt or not set somehow.");
}

void reprodyne_do_not_call_this_function_directly_serialize_video_frame(void* scope,
                                                                        const char* key,
                                                                        const int width,
                                                                        const int height,
                                                                        const int stride,
                                                                        void* bytes)
{
    if(readMode() == Mode::Record)
    {

    }
}

} //extern "C"
