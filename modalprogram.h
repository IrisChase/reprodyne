#pragma once

#include "program.h"

namespace reprodyne
{

struct RecordTape : public Program
{
    void save(const char* path) final;
    double intercept_indeterminate(void* scope, const char* key, const double val) final;
    void serializeCall(void* scope, const char* key, const char* call) final;
};

struct PlayTape : public Program
{
    void save(const char* path) final
    { logic_error_die("Cannot save in playback mode, user code malformed."); }

    double intercept_indeterminate(void* scope, const char* key, const double val) final
    { return playIndeterminate(this, scope, key, val); }

    void serializeCall(void* scope, const char* key, const char* call) final;
};

//Don't touch the indeterminates, just string serial and video.
struct RevalidateTape : public Program
{
    double intercept_indeterminate(void* scope, const char* key, const double val) final
    { return playIndeterminate(this, scope, key, val); }
};


}//reprodyne
