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


std::unique_ptr<reprodyne::ProgramRecorder> recorder;
std::unique_ptr<reprodyne::ProgramPlayer> player;

//Generic reset so I can have n-number of player types and swap between them arbitrarily
static void reset()
{
    recorder.reset();
    player.reset();
}

template<typename T>
T select()
{
    if(recorder) return recorder;
    if(player) return player;
}

extern "C"
{

void reprodyne_do_not_call_this_function_directly_set_playback_failure_handler(Reprodyne_playback_failure_handler handler)
{ reprodyne::State::setPlaybackErrorHandler(handler); /*Not my problem anymore...*/ }

void reprodyne_do_not_call_this_function_directly_record()
{
    reset();
    recorder = std::make_unique<reprodyne::ProgramRecorder>();
}

void reprodyne_do_not_call_this_function_directly_save(const char* path)
{
    if(!recorder) throw std::logic_error("bad doG! No!");
    recorder->save(path);
}

void reprodyne_do_not_call_this_function_directly_play(const char* path)
{
    reset();
    player = std::make_unique<reprodyne::ProgramPlayer>(path);
}

void reprodyne_do_not_call_this_function_directly_assert_complete_read()
{
    //OOOOPS
}

void reprodyne_do_not_call_this_function_directly_open_scope(void* ptr)
{
    if(recorder)    recorder->openScope(ptr);
    else if(player)   player->openScope(ptr);
}

void reprodyne_do_not_call_this_function_directly_mark_frame()
{
    if(recorder)    recorder->markFrame();
    else if(player)   player->markFrame();
}

void reprodyne_do_not_call_this_function_directly_write_indeterminate(void* scopePtr, const char* key, double indeterminate)
{
    if(!recorder) throw std::logic_error("Bad mode. Expected record");
    recorder->intercept(scopePtr, key, indeterminate);
}

double reprodyne_do_not_call_this_function_directly_read_indeterminate(void* scopePtr, const char* subscopeKey)
{
    if(!player) throw std::logic_error("Bad mode. Expected player");
    return player->intercept(scopePtr, subscopeKey, 0); //Value is ignored for read.
}

double reprodyne_do_not_call_this_function_directly_intercept_indeterminate(void* scopePtr, const char* scopeKey, const double indeterminate)
{
    if(recorder)    return recorder->intercept(scopePtr, scopeKey, indeterminate);
    else if(player) return   player->intercept(scopePtr, scopeKey, indeterminate);

    throw std::logic_error("die");
}

void reprodyne_do_not_call_this_function_directly_serialize(void* scopePtr, const char* subScopeKey, const char* call)
{
    //God I wish I had lisp macros...
    if(recorder)    recorder->serialize(scopePtr, subScopeKey, call);
    else if(player)   player->serialize(scopePtr, subScopeKey, call);
    else  throw std::logic_error("die");
}

void reprodyne_do_not_call_this_function_directly_serialize_video_frame(void* scope,
                                                                        const char* key,
                                                                        const int width,
                                                                        const int height,
                                                                        const int stride,
                                                                        void* bytes)
{
}

} //extern "C"
