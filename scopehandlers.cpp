#include "scopehandlers.h"

#include "user-include/reprodyne.h" //For the codes;
#include "errorstuff.h"

namespace reprodyne
{

flatbuffers::Offset<OrdinalScopeTapeEntry> ScopeHandlerRecorder::buildOrdinalScopeFlatbuffer(flatbuffers::FlatBufferBuilder& builder)
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




const KeyedScopeTapeEntry* ScopeHandlerPlayer::getKeyedEntry(const char* subscopeKey)
{
    auto entry = myBuffer->keyedScopeTape()->LookupByKey(subscopeKey);
    if(!entry)
    {
        std::string msg = "Tape empty for key: \"";
        msg += subscopeKey;
        msg += "\"\n";
        throw PlaybackError(REPRODYNE_STAT_EMPTY_TAPE, msg);
    }
    return entry;
}

void ScopeHandlerPlayer::checkReadPastEnd(const int size, const int pos)
{ if(size == pos) throw PlaybackError(REPRODYNE_STAT_TAPE_PAST_END, "Read past end"); }

void ScopeHandlerPlayer::checkFrame(const int frameId1, const int frameId2, const char* moreSpecifically)
{
    std::string msg = "Frame ID mismatch!";
    msg += moreSpecifically;
    msg.push_back('\n');

    if(frameId1 != frameId2) throw PlaybackError(REPRODYNE_STAT_FRAME_MISMATCH, msg);
}

double ScopeHandlerPlayer::intercept(const int frameId, const char* subscopeKey, const double indeterminate)
{
    auto entry = getKeyedEntry(subscopeKey);
    const auto ordinal = readPosMap[entry].indeterminateDoublePos++;

    checkReadPastEnd(entry->programTape()->size(), ordinal);

    auto indeterminateEntry = entry->programTape()->Get(ordinal);

    checkFrame(indeterminateEntry->frameId(), frameId, "Indeterminates out of order!");

    return indeterminateEntry->val();
}

void ScopeHandlerPlayer::serialize(const int frameId, const char* subscopeKey, const char* val)
{
    auto entry = getKeyedEntry(subscopeKey);
    const auto ordinal = readPosMap[entry].serialStringPos++;

    checkReadPastEnd(entry->validationTape()->size(), ordinal);

    auto serialEntry = entry->validationTape()->Get(ordinal);

    checkFrame(serialEntry->frameId(), frameId, "Serialized calls out of order!");

    if(std::string(val) != serialEntry->call()->str())
    {
        std::string msg = "Stored call mismatch! Program produced: ";
        msg += val;
        msg.push_back('\n');
        msg += "Was expecting: \n";
        msg += serialEntry->call()->str();
        msg.push_back('\n');
        throw PlaybackError(REPRODYNE_STAT_CALL_MISMATCH, msg);
    }
}

void ScopeHandlerPlayer::assertCompletReed() //I'm, bored okay?
{
    for(const KeyedScopeTapeEntry* entry : *myBuffer->keyedScopeTape())
    {
        const std::string subscopeKey = entry->key()->str();
        auto readPosIterator = readPosMap.find(entry);

        auto assertEntry = [&](const int progSize, const int callTapeSize)
        {
            auto generateErrorMsg = [&](const std::string type)
            {
                std::string ret;
                ret = type;
                ret = " tape not read to complete for sub scope key: ";
                ret = subscopeKey;
                return ret;
            };

            if(entry->programTape()->size() != progSize)
                throw PlaybackError(REPRODYNE_STAT_PROG_TAPE_INCOMPLETE_READ, generateErrorMsg("Program"));
            if(entry->validationTape()->size() != callTapeSize)
                throw PlaybackError(REPRODYNE_STAT_CALL_TAPE_INCOMPLETE_READ, generateErrorMsg("Call"));
        };

        if(readPosIterator == readPosMap.end()) assertEntry(0, 0); //0 0 because we haven't read anything, obvs
        else assertEntry(readPosIterator->second.indeterminateDoublePos, readPosIterator->second.serialStringPos);
    }
}



}//reprodyne
