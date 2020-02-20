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

    double intercept(const int frameId, const char* subscopeKey, const double indeterminate)
    {
        subScopes[subscopeKey].theDubbles.push_back(CreateIndeterminateDoubleEntry(builder,
                                                                                   frameId,
                                                                                   indeterminate));
        return indeterminate;
    }

    //int intercept(const int frameId, const char* subscopeKey, const int indeterminate)
    //{ return saveValue(subScopes[subscopeKey].theInts, frameId, indeterminate); }

    void serialize(const int frameId, const char* subscopeKey, const char* val)
    {
        subScopes[subscopeKey].serialStrings.push_back(CreateValidationStringEntry(builder,
                                                                                   frameId,
                                                                                   builder.CreateString(val)));
    }

    void serialize(const int frameId, const char* subscopeKey, const int width, const int height, std::vector<int8_t> hash)
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
    void checkFrame(const int frameId1, const int frameId2, const char* moreSpecifically);

public:
    ScopeHandlerPlayer(BufferType* buf): myBuffer(buf) {}

    double intercept(const int frameId, const char* subscopeKey, const double indeterminate);
    //int intercept(const int frameId, const char* subscopeKey, const int indeterminate);

    void serialize(const int frameId, const char* subscopeKey, const char* val);
    void serialize(const int frameId,
                   const char* subscopeKey,
                   const int width,
                   const int height,
                   std::vector<int8_t> hash);

    void assertCompletReed();
};

}//reprodyne
