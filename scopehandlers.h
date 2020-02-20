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
    };

    std::map<std::string, SubScopeEntry> subScopes;

    template<typename ContainerType, typename ValueType>
    ValueType saveValue(ContainerType& cont, const int frameId, const ValueType val)
    {
        cont.emplace_back(frameId, val);
        return cont.back().val;
    }

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

    /*
    void serialize(const int frameId, const char* subscopeKey, const int width, const int height, const char* hash)
    {

    }
    */


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
        int serialStringPos = 0;
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
    void assertCompletReed();
};

}//reprodyne
