#include <string>

#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <assert.h>

#include "schema_generated.h"

#include "lexcompare.h"


std::map<void*, int> scopePtrToOrdinalMap;
std::optional<int> frameCounter;
std::vector<unsigned char> loadedBuffer;

flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();

static int lastFrameId()
{
    if(!frameCounter)
    {
        std::cerr << "Reprodyne ERROR: Call to intercept before reprodyne_mark_frame() is forbidden."
                  << std::endl << std::flush;
        std::terminate();
    }

    return *frameCounter;
}

typedef std::vector<flatbuffers::Offset<Reprodyne::Indeterminate>> IndeterminateTape;
typedef std::map<std::string, IndeterminateTape> SubScopeMap;
typedef std::vector<SubScopeMap> OrdinalScopeTape;

struct PlayRecord
{
    OrdinalScopeTape tape;
    //Serialized Calls synced to frameID..
};

PlayRecord playRecord;

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
const Reprodyne::PlayHistory* flatBuffHistory;

static int readOffset(std::optional<int>& optionalOffset, const int tapeSize)
{
    if(!optionalOffset) //Initialize
    {
        if(tapeSize == 0)
        {
            std::cerr << "Reprodyne INTERNAL ERROR: Tape empty. Bad Tape?" << std::endl << std::flush;
            std::terminate();
        }

        optionalOffset = 0;
    }

    if(*optionalOffset == tapeSize)
    {
        std::cerr << "Reprodyne PLAYBACK FAILURE: Too many reads, tape empty." << std::endl << std::flush;
        std::terminate();
    }

    return (*optionalOffset)++;

}

static void assertFrameId(const int frameId, std::function<void()> handler)
{
    if(frameId == lastFrameId()) return;

    std::cerr << "Reprodyne PLAYBACK FAILURE: Frame ID Mismatch!" << std::endl << std::flush;

    handler();

    //Program state is prolly fucked anyway...
    std::terminate();
}

static double readIndeterminate(void* scope, const std::string subscopeKey)
{
    const int ordinalScopeOffset = scopePtrToOrdinalMap[scope];

    //I just don't feel like naming all the intermediates, bite me.
    const Reprodyne::MinorScopeMap* minorScope =
            flatBuffHistory->scopeTape()->Get(ordinalScopeOffset)->indeterminateTape()->LookupByKey(subscopeKey.c_str());

    const int currentTapeOffset =
            readOffset(lastRead[{ordinalScopeOffset, subscopeKey}].programPos, minorScope->tape()->size());

    const Reprodyne::Indeterminate* i = minorScope->tape()->Get(currentTapeOffset);

    assertFrameId(i->frameId(), [&]
    {
        std::cerr << "Not all determinates consumed on previous frame." << std::endl
                  << "Remaining indeterminates: " << minorScope->tape()->size() - currentTapeOffset << std::endl
                  << "Current frame Id: " << lastFrameId() << std::endl << std::flush;
    });

    return i->invariant();
}

static std::string readStoredCall(void* scope, const std::string subscopeKey)
{
    const int ordinalScopeOffset = scopePtrToOrdinalMap[scope];

    const Reprodyne::MinorScopeMap* minorScope =
            flatBuffHistory->scopeTape()->Get(ordinalScopeOffset)->indeterminateTape()->LookupByKey(subscopeKey.c_str());

    const int currentTapeOffset =
            readOffset(lastRead[{ordinalScopeOffset, subscopeKey}].callPos, minorScope->callTape()->size());

#error Finish meee

}


enum class Mode
{
    Record,
    Play,
};
std::optional<Mode> currentMode;

static Mode readMode()
{
    if(!currentMode)
    {
        std::cerr << "Reprodyne ERROR: Mode not set! Initialize Harness with play(x3th_path) or record()!"
                  << std::endl << std::flush;
        std::terminate();
    }

    return *currentMode;
}

static void init(const Mode theMode)
{
    flatBuffHistory = nullptr;
    currentMode = theMode;
}

extern "C"
{


void reprodyne_record()
{ init(Mode::Record); }


void reprodyne_save(const std::string path)
{
    if(readMode() == Mode::Play)
    {
        std::cerr << "Reprodyne ERROR: Cannot save results in Playback mode!" << std::endl;
        return;
    }


    std::ofstream file(path, std::ios_base::binary);

    std::vector<flatbuffers::Offset<Reprodyne::MajorScope>> extMajorScope;

    for(SubScopeMap& subScopes : playRecord.tape)
    {
        std::vector<flatbuffers::Offset<Reprodyne::MinorScopeMap>> minorScopeFlatMap;

        for(auto pair : subScopes)
        {
            const std::string key = pair.first;
            const IndeterminateTape& tape = pair.second;

            const auto fbString = builder.CreateString(key);
            const auto fbTape = builder.CreateVector(tape);
            const auto minorScopeEntry = Reprodyne::CreateMinorScopeMap(builder, fbString, fbTape);

            minorScopeFlatMap.push_back(minorScopeEntry);
        }

        const auto sortedMinorScopeTable = builder.CreateVectorOfSortedTables(&minorScopeFlatMap);

        const auto majorScopeEntry = Reprodyne::CreateMajorScope(builder, sortedMinorScopeTable);

        extMajorScope.push_back(majorScopeEntry);
    }

    const auto extScopeTape = builder.CreateVector(extMajorScope);
    const auto extPlayHistory = Reprodyne::CreatePlayHistory(builder, extScopeTape);

    builder.Finish(extPlayHistory);

    file.write(reinterpret_cast<char*>(builder.GetBufferPointer()), builder.GetSize());
}

void reprodyne_play(const std::string path)
{
    init(Mode::Play);

    std::ifstream file(path, std::ios_base::binary);

    loadedBuffer = std::vector<unsigned char>(std::istreambuf_iterator(file), {});

    flatBuffHistory = Reprodyne::GetPlayHistory(&loadedBuffer[0]);
}

//If a pointer is reused*, then nothing of note happens.
//It's fine don't worry about it. The Right Thing is done.
//
//(*that is, when a scope is deallocated and you're unfortunate
//  enough that the pointer gets reassigned. If you call this more than
//  once for the same object... Don't do that.)
void reprodyne_open_scope(void* ptr)
{
    playRecord.tape.emplace_back();
    scopePtrToOrdinalMap[ptr] = playRecord.tape.size() - 1;
}

double reprodyne_intercept_indeterminate(void* scopePtr,
                                      const std::string scopeKey,
                                      const double indeterminate)
{
    if(readMode() == Mode::Record)
    {
        auto indeterminateOffset = Reprodyne::CreateIndeterminate(builder,
                                                                         indeterminate,
                                                                         lastFrameId());

        playRecord.tape[scopePtrToOrdinalMap[scopePtr]][scopeKey].push_back(indeterminateOffset);
        return indeterminate;
    }
    else if(readMode() == Mode::Play)
    {
        return readIndeterminate(scopePtr, scopeKey);
    }

    std::cerr << "Reprodyne ERROR: Internal logic error." << std::endl << std::flush;
    std::terminate();
}

void reprodyne_mark_frame()
{
    if(!frameCounter) frameCounter = 0;
    else ++(*frameCounter);
}


void reprodyne_serialize_call(void* scope, const std::string subScopeKey, const std::string call)
{

}

} //extern "C"
