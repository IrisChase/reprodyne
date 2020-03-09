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

#include "programhandlers.h"

#include <fstream>
#include <zlib.h>

#include "fileformat.h"

namespace reprodyne
{

void ProgramRecorder::save(const char* path)
{
    {
        auto ordinalScopes = scopes.pop();

        //TODO: move to ScopeRecorder?
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

    builder = flatbuffers::FlatBufferBuilder(); //reset just in case
}

ProgramPlayer::ProgramPlayer(const char* path)
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
    if(stat != Z_OK)        throw std::runtime_error("Some unknown error occurred during decompression.");

    scopes.load(reprodyne::GetTapeContainer(&loadedBuffer[0])->ordinalScopeTape());
}

}//reprodyne
