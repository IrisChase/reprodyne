#pragma once

#include <vector>
#include <string>
#include <optional>
#include <map>

#include "user-include/reprodyne.h"
#include "schema_generated.h"

#include <fstream>
#include <zlib.h>
#include "fileformat.h"

#include "lexcompare.h"

namespace reprodyne
{

class EmptyTape : public std::exception {};

enum class Mode
{
    Record,
    Play,
};

class ScopeHandlerRecorder
{
    struct SubScopeEntry
    {
        template<typename T>
        struct FrameIdValuePair
        {
            int frameId;
            T val;

            FrameIdValuePair(const int frameId, T val): frameId(frameId), val(val) {}
        };

        std::vector<FrameIdValuePair<double>> theDubbles;
        std::vector<FrameIdValuePair<int>> theInts;
        std::vector<FrameIdValuePair<std::string>> serialStrings;
    };

    std::map<std::string, SubScopeEntry> subScopes;

    template<typename ContainerType, typename ValueType>
    ValueType saveValue(ContainerType& cont, const int frameId, const ValueType val)
    {
        cont.emplace_back(frameId, val);
        return cont.back().val;
    }

public:
    double intercept(const int frameId, const char* subscopeKey, const double indeterminate)
    { return saveValue(subScopes[subscopeKey].theDubbles, frameId, indeterminate); }

    int intercept(const int frameId, const char* subscopeKey, const int indeterminate)
    { return saveValue(subScopes[subscopeKey].theInts, frameId, indeterminate); }


    void serialize(const int frameId, const char* subscopeKey, const char* val)
    { subScopes[subscopeKey].serialStrings.emplace_back(frameId, std::string(val)); }


    flatbuffers::Offset<reprodyne::OrdinalScopeTapeEntry> buildOrdinalScopeFlatbuffer(flatbuffers::FlatBufferBuilder& builder)
    {
        std::vector<flatbuffers::Offset<reprodyne::KeyedScopeTapeEntry>> keyedEntries;
        for(auto pair : subScopes)
        {
            const auto subScopeEntry = pair.second;
            const auto subscopeKey   = builder.CreateString(pair.first);


            std::vector<flatbuffers::Offset<reprodyne::IndeterminateEntry>> indeterminateEntries;
            for(auto entry : subScopeEntry.theDubbles)
                indeterminateEntries.push_back(reprodyne::CreateIndeterminateEntry(builder, entry.frameId, entry.val));

            std::vector<flatbuffers::Offset<reprodyne::CallEntry>> callEntries;
            for(auto entry : subScopeEntry.serialStrings)
                callEntries.push_back(reprodyne::CreateCallEntry(builder, entry.frameId, builder.CreateString(entry.val)));



            keyedEntries.push_back(reprodyne::CreateKeyedScopeTapeEntry(builder,
                                                                        subscopeKey,
                                                                        builder.CreateVector(indeterminateEntries),
                                                                        builder.CreateVector(callEntries)));
        }

        return reprodyne::CreateOrdinalScopeTapeEntry(builder, builder.CreateVectorOfSortedTables(&keyedEntries));
    }
};

class ScopeHandlerPlayer
{
public:
    typedef const OrdinalScopeTapeEntry BufferType;

private:
    const BufferType* myBuffer;

    struct LastReadPos
    {
        int indeterminateDoublePos = 0;
        int serialStringPos = 0;
    };

    std::map<const void*, LastReadPos> readPosMap;

    const KeyedScopeTapeEntry* getKeyedEntry(const char* subscopeKey)
    {
        auto entry = myBuffer->keyedScopeTape()->LookupByKey(subscopeKey);
        if(!entry) throw std::runtime_error("sub scope not found");
        return entry;
    }

    void checkReadPastEnd(const int size, const int pos)
    { if(size == pos) throw std::runtime_error("Read past end"); }

    void checkFrame(const int frameId1, const int frameId2)
    { if(frameId1 != frameId2) throw std::runtime_error("Frame mismatch!"); }

public:
    ScopeHandlerPlayer(BufferType* buf): myBuffer(buf) {}

    double intercept(const int frameId, const char* subscopeKey, const double indeterminate)
    {
        auto entry = getKeyedEntry(subscopeKey);
        const auto ordinal = readPosMap[entry].indeterminateDoublePos++;

        checkReadPastEnd(entry->programTape()->size(), ordinal);

        auto indeterminateEntry = entry->programTape()->Get(ordinal);

        checkFrame(indeterminateEntry->frameId(), frameId);

        return indeterminateEntry->val();
    }

    //int intercept(const int frameId, const char* subscopeKey, const int indeterminate);

    void serialize(const int frameId, const char* subscopeKey, const char* val)
    {
        auto entry = getKeyedEntry(subscopeKey);
        const auto ordinal = readPosMap[entry].serialStringPos++;

        checkReadPastEnd(entry->validationTape()->size(), ordinal);

        auto serialEntry = entry->validationTape()->Get(ordinal);

        checkFrame(serialEntry->frameId(), frameId);

        if(std::string(val) != serialEntry->call()->str()) throw std::runtime_error("Serial call mismatch");
    }
};

class ScopeContainerRecorder
{
    std::vector<ScopeHandlerRecorder> storedScope;
    std::map<void*, int> ordinalMap;

public:
    void openScope(void* ptr)
    {
        storedScope.emplace_back();
        ordinalMap[ptr] = storedScope.size() - 1;
    }

    ScopeHandlerRecorder& at(void* ptr)
    {
        auto ordinalIterator = ordinalMap.find(ptr);
        if(ordinalIterator == ordinalMap.end())
        {
            //todo: throw scope out of range
            throw std::runtime_error("FIXME");
        }

        return storedScope.at(ordinalIterator->second);
    }

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

public:
    void openScope(void *ptr)
    {
        if(ordinalPosition == myRootBuffer->end()) throw std::runtime_error("Ordinal scope buffer overread");

        //TODO: assert the complete read before clobbering?
        scopeMap.insert_or_assign(ptr, ScopeHandlerPlayer(*ordinalPosition));
        ++ordinalPosition;
    }

    ScopeHandlerPlayer& at(void* ptr)
    {
        auto it = scopeMap.find(ptr);
        if(it == scopeMap.end()) throw std::runtime_error("Did not load buffers!");
        return it->second;
    }

    void load(const BufferType* rootBeer)
    {
        myRootBuffer = rootBeer;
        ordinalPosition = myRootBuffer->begin();
    }
};

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
            throw std::runtime_error("TODO: Make this exception proper.");
        return *frameCounter;
    }

    template<typename T>
    T intercept(void* scopePtr, const char* subscopeKey, const T indeterminate)
    { return scopes.at(scopePtr).intercept(readFrameId(), subscopeKey, indeterminate); }

    template<typename T>
    void serialize(void* scopePtr, const char* subscopeKey, const T serialValue)
    { scopes.at(scopePtr).serialize(readFrameId(), subscopeKey, serialValue); }
};

class ProgramRecorder : public Program<ScopeContainerRecorder>
{
public:
    void save(const char* path)
    {
        flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
        {
            auto ordinalScopes = scopes.pop();

            //TODO move to ScopeRecorder?
            std::vector<flatbuffers::Offset<reprodyne::OrdinalScopeTapeEntry>> builtOrdinalEntries;
            for(auto keyedScopeHandler : ordinalScopes)
                    builtOrdinalEntries.push_back(keyedScopeHandler.buildOrdinalScopeFlatbuffer(builder));

            builder.Finish(reprodyne::CreateTapeContainer(builder, builder.CreateVector(builtOrdinalEntries)));
        }

        //0 + to make damn sure the upper bits are zeroed
        uint64_t compressionRegionSize = 0 + compressBound(builder.GetSize());
        std::vector<Bytef> outputBuffer(reprodyne::FileFormat::reservedRangeSize + compressionRegionSize);

        reprodyne::FileFormat::writeBoringStuffToReservedRegion(&outputBuffer[0], compressionRegionSize);

        if(compress(&outputBuffer[reprodyne::FileFormat::compressedDataRegionOffset],
                    &compressionRegionSize,
                    builder.GetBufferPointer(),
                    builder.GetSize())
                != Z_OK) //Heard of A-okay? Yeah well this is like that but not really
        {
            throw std::runtime_error("Some problem with zlib");
        }

        const int finalFileSize = reprodyne::FileFormat::reservedRangeSize + compressionRegionSize;
        std::ofstream(path, std::ios_base::binary).write(reinterpret_cast<char*>(&outputBuffer[0]), finalFileSize);
    }
};

class ProgramPlayer : public Program<ScopeContainerPlayer>
{
    std::vector<unsigned char> loadedBuffer;

public:
    ProgramPlayer(const char* path)
    {
        std::ifstream file(path, std::ios_base::binary);
        auto tempBuffer = std::vector<unsigned char>(std::istreambuf_iterator(file), {});

        if(tempBuffer.size() < reprodyne::FileFormat::reservedRangeSize)
            throw std::runtime_error("Bad file, too small to even hold the reserved region...");

        if(!reprodyne::FileFormat::checkSignature(&tempBuffer[0]))
            throw std::runtime_error("Unrecognized file signature.");

        if(!reprodyne::FileFormat::checkVersion(&tempBuffer[0]))
            throw std::runtime_error("File is not compatible with this version of reprodyne.");

        uint64_t destBuffSize = reprodyne::FileFormat::readUncompressedSize(&tempBuffer[0]);
        loadedBuffer = std::vector<unsigned char>(destBuffSize);

        //READ MOTHERGUCGKer
        const int compressedRegionSize = tempBuffer.size() - reprodyne::FileFormat::reservedRangeSize;
        const auto stat =
                uncompress(&loadedBuffer[0], &destBuffSize,
                           &tempBuffer[reprodyne::FileFormat::compressedDataRegionOffset], compressedRegionSize);

        if(stat == Z_MEM_ERROR) throw std::runtime_error("Not enough memory for zlib.");
        if(stat == Z_BUF_ERROR) throw std::runtime_error("Data corrupt or incomplete.");
        if(stat != Z_OK) throw std::runtime_error("Some unknown error occurred during decompression.");

        scopes.load(reprodyne::GetTapeContainer(&loadedBuffer[0])->ordinalScopeTape());
    }
};


//------------------------------------------------======================$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4 old h-shit

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
