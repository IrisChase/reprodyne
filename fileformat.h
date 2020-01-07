#pragma once

#include <string>
#include <map>

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
    const std::string savedSignature =  std::string(reinterpret_cast<char*>(fileStart), signatureSize);
    const auto mySignature = std::string(&fileSignature[0], signatureSize);
    return savedSignature == mySignature;
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

