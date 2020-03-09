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

#pragma once

#include <vector>
#include <string>
#include <map>

#include "schema_generated.h"


namespace reprodyne
{

class ScopeHandlerRecorder
{
    flatbuffers::FlatBufferBuilder& builder;

    struct SubScopeEntry
    {
        std::vector<flatbuffers::Offset<IndeterminateDoubleEntry>> theDubbles;
        std::vector<flatbuffers::Offset<ValidationStringEntry>> serialStrings;
        std::vector<flatbuffers::Offset<ValidationVideoSHA256Entry>> validationVideoHash;
    };

    std::map<std::string, SubScopeEntry> subScopes;

public:
    ScopeHandlerRecorder(flatbuffers::FlatBufferBuilder& builder): builder(builder) {} //builder builder builder

    double intercept(const unsigned int frameId, const char* subscopeKey, const double indeterminate)
    {
        subScopes[subscopeKey].theDubbles.push_back(CreateIndeterminateDoubleEntry(builder,
                                                                                   frameId,
                                                                                   indeterminate));
        return indeterminate;
    }

    //int intercept(const unsigned int frameId, const char* subscopeKey, const int indeterminate)
    //{ return saveValue(subScopes[subscopeKey].theInts, frameId, indeterminate); }

    void serialize(const unsigned int frameId, const char* subscopeKey, const char* val)
    {
        subScopes[subscopeKey].serialStrings.push_back(CreateValidationStringEntry(builder,
                                                                                   frameId,
                                                                                   builder.CreateString(val)));
    }

    void serialize(const unsigned int frameId,
                   const char* subscopeKey,
                   const int unsigned width,
                   const unsigned int height,
                   std::vector<int8_t> hash)
    {
        subScopes[subscopeKey].validationVideoHash.push_back(CreateValidationVideoSHA256Entry(builder,
                                                                                              frameId,
                                                                                              width,
                                                                                              height,
                                                                                              builder.CreateVector(hash)));
    }


    flatbuffers::Offset<reprodyne::OrdinalScopeTapeEntry> buildOrdinalScopeFlatbuffer(flatbuffers::FlatBufferBuilder& builder);
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
        int validationStringPos = 0;
        int validationVideoHashPos = 0;
    };

    std::map<const KeyedScopeTapeEntry*, LastReadPos> readPosMap;

    const KeyedScopeTapeEntry* getKeyedEntry(const char* subscopeKey);

    void checkReadPastEnd(const int size, const int pos);
    void checkFrame(const unsigned int frameId1, const unsigned int frameId2, const char* moreSpecifically);

public:
    ScopeHandlerPlayer(BufferType* buf): myBuffer(buf) {}

    double intercept(const unsigned int frameId, const char* subscopeKey, const double indeterminate);
    //int intercept(const unsigned int frameId, const char* subscopeKey, const int indeterminate);

    void serialize(const unsigned int frameId, const char* subscopeKey, const char* val);
    void serialize(const unsigned int frameId,
                   const char* subscopeKey,
                   const unsigned int width,
                   const unsigned int height,
                   std::vector<int8_t> hash);

    void assertCompletReed();
};

}//reprodyne
