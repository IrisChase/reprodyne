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

        keyedEntries.push_back(reprodyne::CreateKeyedScopeTapeEntry(builder,
                                                                    subscopeKey,
                                                                    builder.CreateVector(subScopeEntry.theDubbles),
                                                                    builder.CreateVector(subScopeEntry.serialStrings),
                                                                    builder.CreateVector(subScopeEntry.validationVideoHash)));
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

void ScopeHandlerPlayer::checkFrame(const unsigned int frameId1, const unsigned int frameId2, const char* moreSpecifically)
{
    std::string msg = "Frame ID mismatch!";
    msg += moreSpecifically;
    msg.push_back('\n');

    if(frameId1 != frameId2) throw PlaybackError(REPRODYNE_STAT_FRAME_MISMATCH, msg);
}

double ScopeHandlerPlayer::intercept(const unsigned int frameId, const char* subscopeKey, const double indeterminate)
{
    auto entry = getKeyedEntry(subscopeKey);
    const auto ordinal = readPosMap[entry].indeterminateDoublePos++;

    checkReadPastEnd(entry->indeterminateDoubles()->size(), ordinal);

    auto indeterminateEntry = entry->indeterminateDoubles()->Get(ordinal);

    checkFrame(indeterminateEntry->frameId(), frameId, "Indeterminates out of order!");

    return indeterminateEntry->val();
}

void ScopeHandlerPlayer::serialize(const unsigned int frameId, const char* subscopeKey, const char* val)
{
    auto entry = getKeyedEntry(subscopeKey);
    const auto ordinal = readPosMap[entry].validationStringPos++;

    checkReadPastEnd(entry->validationStrings()->size(), ordinal);

    auto serialEntry = entry->validationStrings()->Get(ordinal);

    checkFrame(serialEntry->frameId(), frameId, "Serialized calls out of order!");

    if(std::string(val) != serialEntry->str()->str())
    {
        std::string msg = "Stored call mismatch! Program produced: ";
        msg += val;
        msg.push_back('\n');
        msg += "Was expecting: \n";
        msg += serialEntry->str()->str();
        msg.push_back('\n');
        throw PlaybackError(REPRODYNE_STAT_VALIDATION_FAIL, msg);
    }
}

void ScopeHandlerPlayer::serialize(const unsigned int frameId,
                                   const char* subscopeKey,
                                   const unsigned int width,
                                   const unsigned int height,
                                   std::vector<int8_t> hash)
{
    auto entry = getKeyedEntry(subscopeKey);
    const auto ordinal = readPosMap[entry].validationVideoHashPos++;

    checkReadPastEnd(entry->validationVideoSHA256Hashes()->size(), ordinal);

    auto serialEntry = entry->validationVideoSHA256Hashes()->Get(ordinal);

    checkFrame(serialEntry->frameId(), frameId, "Validation video frame out of order!");

    if(!std::equal(hash.begin(), hash.end(), serialEntry->sha256()->begin(), serialEntry->sha256()->end()))
        throw PlaybackError(REPRODYNE_STAT_VALIDATION_FAIL, "Stored video hash mismatch!");
}

void ScopeHandlerPlayer::assertCompletReed() //I'm, bored okay?
{
    for(const KeyedScopeTapeEntry* entry : *myBuffer->keyedScopeTape())
    {
        const std::string subscopeKey = entry->key()->str();
        auto readPosIterator = readPosMap.find(entry);

        auto assertEntry = [&](const LastReadPos& pos)
        {
            auto generateErrorMsg = [&](const std::string type)
            {
                std::string ret;
                ret = type;
                ret = " tape not read to complete for sub scope key: ";
                ret = subscopeKey;
                return ret;
            };

            if(entry->indeterminateDoubles()->size() != pos.indeterminateDoublePos)
                throw PlaybackError(REPRODYNE_STAT_PROG_TAPE_INCOMPLETE_READ, generateErrorMsg("Program"));

            if(entry->validationStrings()->size() != pos.validationStringPos)
                throw PlaybackError(REPRODYNE_STAT_CALL_TAPE_INCOMPLETE_READ, generateErrorMsg("Call"));
            if(entry->validationVideoSHA256Hashes()->size() != pos.validationVideoHashPos)
                throw PlaybackError(REPRODYNE_STAT_CALL_TAPE_INCOMPLETE_READ, generateErrorMsg("Video hash"));
        };

        if(readPosIterator == readPosMap.end()) assertEntry(LastReadPos()); //Empty read pos because we haven't read anything, obvs
        else assertEntry(readPosIterator->second);
    }
}



}//reprodyne
