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

Reprodyne_playback_failure_handler playbackErrorHandler = nullptr;
std::string jumpSafeString;

static void handlePlaybackErrorException(const reprodyne::PlaybackError& e)
{
    if(!playbackErrorHandler)
    {
        std::cerr << "Reprodyne PLAYBACK FAILURE (No handler provided): " << e.what() << std::endl << std::flush;
        std::terminate();
    }

    jumpSafeString = e.what(); //what() references an std::string, so it's not safe beyond this point.
    playbackErrorHandler(e.getCode(), jumpSafeString.c_str());
}

static void handleRuntimeErrrr(const char* what)
{
    std::cerr << "Reprodyne runtime error: " << what << std::endl;
}

static void unknownErrorDie()
{
    std::terminate();
}

template<typename T>
static T safeBlock(std::function<T()> thingamajigulator)
{
    try
    { return thingamajigulator(); }
    catch(const reprodyne::PlaybackError& e)
    { handlePlaybackErrorException(e); }
    catch(const std::runtime_error& e)
    { handleRuntimeErrrr(e.what()); }
    catch(...)
    {
        //fucc
        unknownErrorDie();
    }
}

template<>
void safeBlock(std::function<void()> thingamajigulator)
{
    try
    { thingamajigulator(); }
    catch(const reprodyne::PlaybackError& e)
    { handlePlaybackErrorException(e); }
    catch(const std::runtime_error& e)
    { handleRuntimeErrrr(e.what()); }
    catch(...)
    {
        //fucc
        unknownErrorDie();
    }
}

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
{ playbackErrorHandler = handler; /*Not my problem anymore...*/ }

void reprodyne_do_not_call_this_function_directly_record()
{
    safeBlock<void>([&]
    {
        reset();
        recorder = std::make_unique<reprodyne::ProgramRecorder>();
    });
}

void reprodyne_do_not_call_this_function_directly_save(const char* path)
{
    safeBlock<void>([&]
    {
        if(!recorder) throw std::logic_error("bad doG! No!");
        recorder->save(path);
    });
}

void reprodyne_do_not_call_this_function_directly_play(const char* path)
{
    safeBlock<void>([&]
    {
        reset();
        player = std::make_unique<reprodyne::ProgramPlayer>(path);
    });
}

void reprodyne_do_not_call_this_function_directly_assert_complete_read()
{
    //OOOOPS
    safeBlock<void>([&]
    {

    });
}

void reprodyne_do_not_call_this_function_directly_open_scope(void* ptr)
{
    safeBlock<void>([&]
    {
        if(recorder)    recorder->openScope(ptr);
        else if(player)   player->openScope(ptr);
    });
}

void reprodyne_do_not_call_this_function_directly_mark_frame()
{
    safeBlock<void>([&]
    {
        if(recorder)    recorder->markFrame();
        else if(player)   player->markFrame();
    });
}

void reprodyne_do_not_call_this_function_directly_write_indeterminate(void* scopePtr, const char* key, double indeterminate)
{
    safeBlock<void>([&]
    {
        if(!recorder) throw std::logic_error("Bad mode. Expected record");
        recorder->intercept(scopePtr, key, indeterminate);
    });
}

double reprodyne_do_not_call_this_function_directly_read_indeterminate(void* scopePtr, const char* subscopeKey)
{
    return safeBlock<double>([&]
    {
        if(!player) throw std::logic_error("Bad mode. Expected player");
        return player->intercept(scopePtr, subscopeKey, 0); //Value is ignored for read.
    });
}

double reprodyne_do_not_call_this_function_directly_intercept_indeterminate(void* scopePtr, const char* scopeKey, const double indeterminate)
{
    return safeBlock<double>([&]()
    {
        if(recorder)    return recorder->intercept(scopePtr, scopeKey, indeterminate);
        else if(player) return   player->intercept(scopePtr, scopeKey, indeterminate);

        throw std::logic_error("die");
    });
}

void reprodyne_do_not_call_this_function_directly_serialize(void* scopePtr, const char* subScopeKey, const char* call)
{
    return safeBlock<void>([&]
    {
        //God I wish I had lisp macros...
        if(recorder)    recorder->serialize(scopePtr, subScopeKey, call);
        else if(player)   player->serialize(scopePtr, subScopeKey, call);
        else  throw std::logic_error("die");
    });
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
