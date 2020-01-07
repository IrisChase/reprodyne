#pragma once

#include <string>
#include <vector>
#include <map>

#include "schema_generated.h"

namespace reprodyne
{

struct Live
{
    typedef std::vector<flatbuffers::Offset<reprodyne::IndeterminateEntry>> IndeterminateTape;
    typedef std::vector<flatbuffers::Offset<reprodyne::CallEntry>> CallTape;

    struct VideoTapeEntry
    {
        std::string codec;
        std::vector<unsigned char> data;
        unsigned int width;
        unsigned int height;
    };

    typedef std::vector<VideoTapeEntry> OrdinalVideoTapes;

    struct KeyedScopeEntry
    {
        IndeterminateTape programTape;
        CallTape validationTape;
        OrdinalVideoTapes videoTape;
    };

    typedef std::map<std::string, KeyedScopeEntry> LiveKeyedScopeMap;
    typedef std::vector<LiveKeyedScopeMap> LiveOrdinalScopeTape;

    void open_scope(void* ptr)
    {

    }

};

extern Live liveData;

}//reprodyne
