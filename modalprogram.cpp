#include "modalprogram.h"

namespace reprodyne
{

double RecordTape::intercept_indeterminate(void* scope, const char* key, const double val)
{
    return recordIndeterminate(this, scope, key, val);
}

void RecordTape::serializeCall(void* scope, const char* key, const char* call)
{
    const int scopeOrdinal = readScopeOrdinal(scope);
    const std::string cppStringCall = call; //Just so we can size it
    auto stringOffset = builder.CreateString(cppStringCall.c_str(), cppStringCall.size());
    auto callEntryOffset = reprodyne::CreateCallEntry(builder, lastFrameId(), stringOffset);

    liveTape[scopeOrdinal][key].validationTape.push_back(callEntryOffset);
}

void PlayTape::serializeCall(void* scope, const char* key, const char* call)
{
    const int ordinalScopeOffset = readScopeOrdinal(scope);
    const auto validationTape = readKeyedScopeTapeEntry(ordinalScopeOffset,
                                                             jumpSafeString.c_str(),
                                                             "Call")->validationTape();

    if(!validationTape)
    {
        jumpSafeString = "Validation tape empty for key: ";
        jumpSafeString += key;
        jumpSafeString.push_back('\n');
        playback_error_handler_wrapper(REPRODYNE_STAT_EMPTY_TAPE, jumpSafeString.c_str());
    }

    const reprodyne::CallEntry* c =
            validationTape->Get(readOffset(lastRead[{ordinalScopeOffset, key}].callPos,
                                                validationTape->size()));

    assertFrameId(c->frameId(), "Serialized calls out of order!");
    jumpSafeString = c->call()->str();

    bool failure = false;

    //This is convoluted because we need to ensure that the destructor for
    // "storedCall" is invoked before we call the playback_error_handler_wrapper,
    // which may longjmp back out of here.
    if(jumpSafeString != call)
    {
        failure = true;

        const std::string storedCall = jumpSafeString;
        jumpSafeString = "Stored call mismatch! Program produced: \n";
        jumpSafeString += call;
        jumpSafeString += "Was expecting: \n";
        jumpSafeString += storedCall;
        jumpSafeString += '\n';
    }

    if(failure) playback_error_handler_wrapper(REPRODYNE_STAT_CALL_MISMATCH,
                                                    jumpSafeString.c_str());
}



}//reprodyne
