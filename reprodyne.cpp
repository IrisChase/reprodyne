#include <string>

#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <assert.h>

#include <zlib.h>

#include <libavcodec/avcodec.h>

#include "lexcompare.h"

#include "schema_generated.h"

#include "user-include/reprodyne.h"
#include "state.h"

reprodyne::State currentState;

extern "C"
{

void reprodyne_do_not_call_this_function_directly_set_playback_failure_handler(Reprodyne_playback_failure_handler handler)
{ reprodyne::State::setPlaybackErrorHandler(handler); /*Not my problem anymore...*/ }

void reprodyne_do_not_call_this_function_directly_record()
{ currentState = reprodyne::State(reprodyne::Mode::Record); }

void reprodyne_do_not_call_this_function_directly_save(const char* path)
{ currentState.save(path); }

void reprodyne_do_not_call_this_function_directly_play(const char* path)
{
    currentState = reprodyne::State(reprodyne::Mode::Play);
    currentState.load(path);
}

void reprodyne_do_not_call_this_function_directly_assert_complete_read()
{ currentState.assertCompleteRead(); }

void reprodyne_do_not_call_this_function_directly_open_scope(void* ptr)
{ currentState.openScope(ptr); }

void reprodyne_do_not_call_this_function_directly_mark_frame()
{ currentState.markFrame(); }

void reprodyne_do_not_call_this_function_directly_write_indeterminate(void* scopePtr, const char* key, double indeterminate)
{ currentState.writeIndeterminate(scopePtr, key, indeterminate); }

double reprodyne_do_not_call_this_function_directly_read_indeterminate(void* scopePtr, const char* subscopeKey)
{ return currentState.readIndeterminate(scopePtr, subscopeKey); }

double reprodyne_do_not_call_this_function_directly_intercept_indeterminate(void* scopePtr, const char* scopeKey, const double indeterminate)
{
    if(currentState.readMode() == reprodyne::Mode::Record)
    {
        reprodyne_do_not_call_this_function_directly_write_indeterminate(scopePtr, scopeKey, indeterminate);
        return indeterminate;
    }
    else if(currentState.readMode() == reprodyne::Mode::Play)
    {
        return reprodyne_do_not_call_this_function_directly_read_indeterminate(scopePtr, scopeKey);
    }

    currentState.logic_error_die();
}

void reprodyne_do_not_call_this_function_directly_serialize(void* scopePtr, const char* subScopeKey, const char* call)
{ currentState.serializeCall(scopePtr, subScopeKey, call); }

void reprodyne_do_not_call_this_function_directly_serialize_video_frame(void* scope,
                                                                        const char* key,
                                                                        const int width,
                                                                        const int height,
                                                                        const int stride,
                                                                        void* bytes)
{
    if(currentState.readMode() == reprodyne::Mode::Record)
    {

    }
}

} //extern "C"
