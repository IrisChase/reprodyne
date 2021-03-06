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

#include <iostream>


#include <openssl/sha.h>

#include "user-include/reprodyne.h"

#include "programhandlers.h"


std::unique_ptr<reprodyne::ProgramRecorder> recorder;
std::unique_ptr<reprodyne::ProgramPlayer> player;

reprodyne_playback_failure_handler playbackErrorHandler = nullptr;
std::string jumpSafeString;

//(re: using std::strings in exceptions)
//Playing it on the loose and easy with these exceptions because they are playback,
// not memory errors. And it's prettier this way~


//We need to catch all exceptions and deal with them here because you don't want
// the library to leak through a C boundary. And as such, we want to be longjmp safe.
static void handlePlaybackErrorException(const reprodyne::PlaybackError& e)
{
    if(!playbackErrorHandler)
    {
        std::cerr << "Reprodyne PLAYBACK FAILURE (No handler provided): " << e.what() << std::endl << std::flush;
        std::terminate();
    }

    jumpSafeString = e.what(); //what() references an std::string, so it's not safe beyond this point.
    playbackErrorHandler(e.getCode(), jumpSafeString.c_str());
    std::terminate(); //If you don't do it, I will.
}

static void handleRuntimeErrrr(const char* what)
{
    std::cerr << "Reprodyne runtime error: " << what << std::endl;
    std::terminate();
}

static void unknownErrorDie()
{
    std::terminate();
}

//Uniform exception handler so that we can translate to C-safe errors consistently.
template<typename T>
static T safeBlock(std::function<T()> thingamajigulator)
{
    try
    { return thingamajigulator(); }
    catch(const reprodyne::PlaybackError& e)
    { handlePlaybackErrorException(e); }
    catch(const std::runtime_error& e)
    { handleRuntimeErrrr(e.what()); } //"eh, what??"
    catch(...)
    {
        //fucc
        unknownErrorDie();
    }

    std::terminate(); //Why won't you fuCKING DIE ALREADY?!?
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

extern "C"
{

void reprodyne_do_not_call_this_function_directly_set_playback_failure_handler(reprodyne_playback_failure_handler handler)
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
    safeBlock<void>([&]
    {
        if(!player) throw std::logic_error("Cannot assert complete read in record mode");
        player->assertCompleteRead();
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

void reprodyne_do_not_call_this_function_directly_open_scope(void* ptr)
{
    safeBlock<void>([&]
    {
        if(recorder)    recorder->openScope(ptr);
        else if(player)   player->openScope(ptr);
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

double reprodyne_do_not_call_this_function_directly_intercept_double(void* scopePtr, const char* scopeKey, const double indeterminate)
{
    return safeBlock<double>([&]()
    {
        if(recorder)    return recorder->intercept(scopePtr, scopeKey, indeterminate);
        else if(player) return   player->intercept(scopePtr, scopeKey, indeterminate);

        throw std::logic_error("die");
    });
}

void reprodyne_do_not_call_this_function_directly_validate_string(void* scopePtr, const char* subScopeKey, const char* call)
{
    return safeBlock<void>([&]
    {
        //God I wish I had lisp macros...
        if(recorder)    recorder->serialize(scopePtr, subScopeKey, call);
        else if(player)   player->serialize(scopePtr, subScopeKey, call);
        else  throw std::logic_error("die");
    });
}

void reprodyne_do_not_call_this_function_directly_validate_bitmap_hash(void* scope,
                                                                       const char* key,
                                                                       const unsigned int width,
                                                                       const unsigned int height,
                                                                       const unsigned int stride,
                                                                       void* bytes)
{
    safeBlock<void>([&]
    {
        //I like my typos sometimes
        auto errorHahnDler = [&](const char* msg)
        {
            //This is only safe because jumpSafeString is only otherwise used by
            // the playback error handler. Even still, I don't like this...
            jumpSafeString = msg;
            jumpSafeString += " in validate bitmap hash function!";
            jumpSafeString += " Subscope key: ";
            jumpSafeString += key;
            throw std::runtime_error(jumpSafeString.c_str());
        };

        if(height < 1)      errorHahnDler("Useless height");
        if(width < 1)       errorHahnDler("Useless width");
        if(stride < width)  errorHahnDler("Stride smaller than width");
        if(!bytes)          errorHahnDler("NULL data pointer");

        //int8_t because that's how flatbuffers likes it, I ain't judge.
        std::vector<int8_t> hash(SHA256_DIGEST_LENGTH);

        //unsigned char because that's how OpenSSL likes it, I ain't judge.
        std::vector<unsigned char> clippedImageRegion;
        clippedImageRegion.reserve(width * height);

        for(int h = 0; h != height; ++h)
            for(int w = 0; w != width; ++w)
                clippedImageRegion.push_back(reinterpret_cast<unsigned char*>(bytes)[h * stride + w]);

        SHA256(&clippedImageRegion[0], clippedImageRegion.size(), reinterpret_cast<unsigned char*>(&hash[0]));

        if(recorder)    recorder->serialize(scope, key, width, height, hash); //Variadic templates are badass.
        else if(player)   player->serialize(scope, key, width, height, hash);
    });
}

} //extern "C"
