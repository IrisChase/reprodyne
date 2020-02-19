#ifndef REPRODYNE_H
#define REPRODYNE_H

#define REPRODYNE_STAT_FRAME_MISMATCH              100
#define REPRODYNE_STAT_CALL_MISMATCH               101
#define REPRODYNE_STAT_EMPTY_TAPE                  102
#define REPRODYNE_STAT_TAPE_PAST_END               103
#define REPRODYNE_STAT_CALL_TAPE_INCOMPLETE_READ   104
#define REPRODYNE_STAT_PROG_TAPE_INCOMPLETE_READ   105
#define REPRODYNE_STAT_UNREGISTERED_SCOPE          106

typedef void(*reprodyne_playback_failure_handler)(const int code, const char* msg);

#ifdef REPRODYNE_AVAILABLE

//If you believe lower case macros are evil... I'm sorry.
//I just... I can't yell this much...

#define reprodyne_set_playback_failure_handler(handler) \
    reprodyne_do_not_call_this_function_directly_set_playback_failure_handler(handler)

#define reprodyne_record() reprodyne_do_not_call_this_function_directly_record()
#define reprodyne_save(path) reprodyne_do_not_call_this_function_directly_save(path)
#define reprodyne_play(path) reprodyne_do_not_call_this_function_directly_play(path)

#define reprodyne_assert_complete_read() reprodyne_do_not_call_this_function_directly_assert_complete_read()

//If a pointer is reused*, then nothing of note happens.
//It's fine don't worry about it. The Right Thing is done.
//
//(*that is, when a scope is deallocated and you're unfortunate
//  enough that the pointer gets reassigned. If you call this more than
//  once for the same object... Don't do that.)
#define reprodyne_open_scope(scope) reprodyne_do_not_call_this_function_directly_open_scope(scope)
#define reprodyne_mark_frame() reprodyne_do_not_call_this_function_directly_mark_frame()

#define reprodyne_intercept_indeterminate_double(scope, key, val) \
    reprodyne_do_not_call_this_function_directly_intercept_indeterminate_double(scope, key, val)

#define reprodyne_validate_string(scope, key, call) \
    reprodyne_do_not_call_this_function_directly_validate_string(scope, key, call)

#define reprodyne_validate_video_frame_hash(scope, key, width, height, stride, bytes) \
    reprodyne_do_not_call_this_function_directly_validate_video_frame_hash(scope, key, width, height, stride, byes)


/*-------------------------------Implementation junk beyond this point-------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

void reprodyne_do_not_call_this_function_directly_set_playback_failure_handler(reprodyne_playback_failure_handler handler);

void reprodyne_do_not_call_this_function_directly_record();
void reprodyne_do_not_call_this_function_directly_save(const char* path);

void reprodyne_do_not_call_this_function_directly_play(const char* path);

void reprodyne_do_not_call_this_function_directly_open_scope(void* scope);
void reprodyne_do_not_call_this_function_directly_mark_frame();

double reprodyne_do_not_call_this_function_directly_intercept_indeterminate_double(void* scope,
                                                                                   const char* key,
                                                                                   double val);

void reprodyne_do_not_call_this_function_directly_validate_string(void* scope, const char* key, const char* cereal);

void reprodyne_do_not_call_this_function_directly_validate_video_frame_hash(void* scope,
                                                                            const char* key,
                                                                            const int width,
                                                                            const int height,
                                                                            const int stride,
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

#define reprodyne_intercept_indeterminate_double(scope, key, val) val

#define reprodyne_validate_string(scope, key, call) ((void)0)
#define reprodyne_validate_video_frame_hash(scope, key, width, height, stride, bytes) ((void)0)


#endif //REPRODYNE_AVAILABLE
#endif //REPRODYNE_H
