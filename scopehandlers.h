#pragma once

#include <vector>
#include <string>
#include <map>

#include "schema_generated.h"


namespace reprodyne
{

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
