#include "tapemode.h"

namespace reprodyne
{

void recordSerialCall(Program* com, void* scope, const char* key, const char* call)
{
    const int scopeOrdinal = com->readScopeOrdinal(scope);
    const std::string cppStringCall = call; //Just so we can size it
    auto stringOffset = com->builder.CreateString(cppStringCall.c_str(), cppStringCall.size());
    auto callEntryOffset = reprodyne::CreateCallEntry(com->builder, com->lastFrameId(), stringOffset);

    com->liveTape[scopeOrdinal][key].validationTape.push_back(callEntryOffset);
}

void validateSerialCall(Program* com, void* scope, const char* key, const char* call)
{
    const int ordinalScopeOffset = com->readScopeOrdinal(scope);
    const auto validationTape = com->readKeyedScopeTapeEntry(ordinalScopeOffset,
                                                             com->jumpSafeString.c_str(),
                                                             "Call")->validationTape();

    if(!validationTape)
    {
        com->jumpSafeString = "Validation tape empty for key: ";
        com->jumpSafeString += key;
        com->jumpSafeString.push_back('\n');
        com->playback_error_handler_wrapper(REPRODYNE_STAT_EMPTY_TAPE, com->jumpSafeString.c_str());
    }

    const reprodyne::CallEntry* c =
            validationTape->Get(com->readOffset(com->lastRead[{ordinalScopeOffset, key}].callPos,
                                                validationTape->size()));

    com->assertFrameId(c->frameId(), "Serialized calls out of order!");
    com->jumpSafeString = c->call()->str();

    bool failure = false;

    //This is convoluted because we need to ensure that the destructor for
    // "storedCall" is invoked before we call the playback_error_handler_wrapper,
    // which may longjmp back out of here.
    if(com->jumpSafeString != call)
    {
        failure = true;

        const std::string storedCall = com->jumpSafeString;
        com->jumpSafeString = "Stored call mismatch! Program produced: \n";
        com->jumpSafeString += call;
        com->jumpSafeString += "Was expecting: \n";
        com->jumpSafeString += storedCall;
        com->jumpSafeString += '\n';
    }

    if(failure) com->playback_error_handler_wrapper(REPRODYNE_STAT_CALL_MISMATCH,
                                                    com->jumpSafeString.c_str());
}

}//reprodyne
