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

#include <string>

namespace reprodyne
{
namespace FileFormat
{

//For 8 * 8 == 64 bit or enough for a 138,547,332 terrabyte file or versions... Should be enough for everybody.
//Integers are stored little endian because I felt like it
const int intSize = 8;

//"x3th", encoded as ascii. Manually, because I don't trust locales.
const int signatureOffset = 0;
const int signatureSize = 4;
const char fileSignature[signatureSize] = {0x78, 0x33, 0x74, 0x68};

const int versionOffset = signatureOffset + signatureSize;
const int versionSize = intSize;

const int currentFileFormatVersion = 0x01; //Good thing we used a 64 bit integer for this

const int uncompressedSizeOffset = versionOffset + versionSize;
const int uncompressedSizeSize = intSize;

const int compressedDataRegionOffset = uncompressedSizeOffset + uncompressedSizeSize;

const int reservedRangeSize = compressedDataRegionOffset;

inline uint64_t readInteger(unsigned char* pos)
{
    uint64_t ret = 0;
    for(int i = 0; i != intSize; ++i) ret = uint64_t(pos[i]) << i * 8 | ret;
    return ret;
}

inline void writeInteger(unsigned char* pos, const uint64_t val)
{
    for(int i = 0; i != intSize; ++i)
        pos[i] = val >> i * 8;
}

inline void writeVersion(unsigned char* fileStart)
{
    writeInteger(&fileStart[versionOffset], currentFileFormatVersion);
}

inline bool checkVersion(unsigned char* fileStart)
{
    return readInteger(&fileStart[versionOffset]) == currentFileFormatVersion;
}

inline void writeSignature(unsigned char* fileStart)
{
    for(int i = 0; i != signatureSize; ++i)
        fileStart[i] = fileSignature[i];
}

inline bool checkSignature(unsigned char* fileStart)
{
    return std::string(reinterpret_cast<char*>(fileStart), signatureSize)
        == std::string(&fileSignature[0], signatureSize);
}

inline uint64_t readUncompressedSize(unsigned char* fileStart)
{
    return readInteger(&fileStart[uncompressedSizeOffset]);
}

inline void writeBoringStuffToReservedRegion(unsigned char* filesStart, const uint64_t uncompressedSize)
{
    writeSignature(filesStart);
    writeVersion(filesStart);
    writeInteger(&filesStart[uncompressedSizeOffset], uncompressedSize);
}

}//FileFormat
}//reprodyne

