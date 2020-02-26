#ifndef REPRODYNE_H
#define REPRODYNE_H

#define REPRODYNE_STAT_FRAME_MISMATCH              100
#define REPRODYNE_STAT_VALIDATION_FAIL             101
#define REPRODYNE_STAT_EMPTY_TAPE                  102
#define REPRODYNE_STAT_TAPE_PAST_END               103
#define REPRODYNE_STAT_CALL_TAPE_INCOMPLETE_READ   104
#define REPRODYNE_STAT_PROG_TAPE_INCOMPLETE_READ   105
#define REPRODYNE_STAT_UNREGISTERED_SCOPE          106

typedef void(*reprodyne_playback_failure_handler)(const int code, const char* msg);

#ifdef REPRODYNE_AVAILABLE

//Does what you'd expect, set the playback handler (Using the above function signature)
// that will be called if there's anything fishy with the playback. May also be called
// if something illegal happens in record mode.
#define reprodyne_set_playback_failure_handler(handler) \
    reprodyne_do_not_call_this_function_directly_set_playback_failure_handler(handler)

//Called to initialize recording.
#define reprodyne_record() reprodyne_do_not_call_this_function_directly_record()

//Save recording to path.
#define reprodyne_save(path) reprodyne_do_not_call_this_function_directly_save(path)

//Called to initialize playback.
#define reprodyne_play(path) reprodyne_do_not_call_this_function_directly_play(path)

//Mark a "frame", this must be called before any validator or interceptors. Suggested
// to be called at the top of a main loop, if applicable. Otherwise once at the beginning of
// the (presumably) short lived procedure.
#define reprodyne_mark_frame() reprodyne_do_not_call_this_function_directly_mark_frame()

//Open scope before use with interceptors or validators. A reused pointer will shadow it's
// previous scope. Pointer reuse need not be deterministic (They can be reused in one run but not
// another, for example), only the order of allocation, and what the pointer represents
// at any given time is important.
#define reprodyne_open_scope(scope) reprodyne_do_not_call_this_function_directly_open_scope(scope)

//Intercept a double against the scope/key pair.
#define reprodyne_intercept_double(scope, key, val) \
    reprodyne_do_not_call_this_function_directly_intercept_double(scope, key, val)

//Validate a string against the scope/key pair.
#define reprodyne_validate_string(scope, key, call) \
    reprodyne_do_not_call_this_function_directly_validate_string(scope, key, call)

//Validate a bitmap against a scope/key pair by taking a hash.
// NOTE: Width and stride represent the width of you data in *BYTES*
//It's fine if the stride is the same size as the width, this should be useful for generic data.
#define reprodyne_validate_bitmap_hash(scope, key, width, height, stride, bytes) \
    reprodyne_do_not_call_this_function_directly_validate_bitmap_hash(scope, key, width, height, stride, bytes)

//Make sure that the program didn't terminate early, and that all the validation
// and indeterminate tapes have been read to completion. Only makes sense in playback mode.
#define reprodyne_assert_complete_read() reprodyne_do_not_call_this_function_directly_assert_complete_read()


//If you believe lower case macros are evil... I'm sorry.
//I just... I can't yell this much...

/*-------------------------------Implementation junk beyond this point-------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

void reprodyne_do_not_call_this_function_directly_set_playback_failure_handler(reprodyne_playback_failure_handler handler);

void reprodyne_do_not_call_this_function_directly_record();
void reprodyne_do_not_call_this_function_directly_save(const char* path);

void reprodyne_do_not_call_this_function_directly_play(const char* path);

void reprodyne_do_not_call_this_function_directly_mark_frame();
void reprodyne_do_not_call_this_function_directly_open_scope(void* scope);

double reprodyne_do_not_call_this_function_directly_intercept_double(void* scope,
                                                                     const char* key,
                                                                     double val);

void reprodyne_do_not_call_this_function_directly_validate_string(void* scope, const char* key, const char* cereal);

void reprodyne_do_not_call_this_function_directly_validate_bitmap_hash(void* scope,
                                                                       const char* key,
                                                                       const unsigned int width,
                                                                       const unsigned int height,
                                                                       const unsigned int stride,
                                                                       void* bytes);

void reprodyne_do_not_call_this_function_directly_assert_complete_read();

#ifdef __cplusplus
} //extern "C"
#endif

#else

#define reprodyne_set_playback_failure_handler(handler) ((void)0)

#define reprodyne_mode_play() ((void)0)
#define reprodyne_mode_record() ((void)0)

#define reprodyne_record() ((void)0)
#define reprodyne_save(path) ((void)0)
#define reprodyne_play(path) ((void)0)

#define reprodyne_assert_complete_read() ((void)0)

#define reprodyne_open_scope(scope) ((void)0)
#define reprodyne_mark_frame() ((void)0)

#define reprodyne_intercept_double(scope, key, val) val

#define reprodyne_validate_string(scope, key, call) ((void)0)
#define reprodyne_validate_bitmap_hash(scope, key, width, height, stride, bytes) ((void)0)


#endif //REPRODYNE_AVAILABLE
#endif //REPRODYNE_H
