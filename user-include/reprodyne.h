#ifndef REPRODYNE_H
#define REPRODYNE_H

typedef void(*Reprodyne_playback_failure_handler)(const char* msg);

#ifdef REPRODYNE_AVAILABLE

extern "C"
{

void reprodyne_internal_reset();

void reprodyne_internal_set_playback_failure_handler(Reprodyne_playback_failure_handler handler);

int reprodyne_internal_mode_play();
int reprodyne_internal_mode_record();

void reprodyne_internal_record();
void reprodyne_internal_save(const char* path);
void reprodyne_internal_play(const char* path);

void reprodyne_internal_open_scope(void* scope);
void reprodyne_internal_mark_frame();

void reprodyne_internal_write_indeterminate(void* scope, const char* key, double val);
double reprodyne_internal_read_indeterminate(void* scope, const char* key);
double reprodyne_internal_intercept_indeterminate(void* scope, const char* key, double val);

void reprodyne_internal_serialize(void* scope, const char* key, const char* cereal);

} //extern "C"

//If you believe lower case macros are evil... I'm sorry.
//I just... I can't yell this much...

#define reprodyne_reset() reprodyne_internal_reset()

#define reprodyne_set_playback_failure_handler(handler) reprodyne_internal_set_playback_failure_handler(handler)

#define reprodyne_mode_play() reprodyne_internal_mode_play()
#define reprodyne_mode_record() reprodyne_internal_mode_record()

#define reprodyne_record() reprodyne_internal_record()
#define reprodyne_save(path) reprodyne_internal_save(path)
#define reprodyne_play(path) reprodyne_internal_play(path)

//If a pointer is reused*, then nothing of note happens.
//It's fine don't worry about it. The Right Thing is done.
//
//(*that is, when a scope is deallocated and you're unfortunate
//  enough that the pointer gets reassigned. If you call this more than
//  once for the same object... Don't do that.)
#define reprodyne_open_scope(scope) reprodyne_internal_open_scope(scope)
#define reprodyne_mark_frame() reprodyne_internal_mark_frame()

#define reprodyne_write_indeterminate(scope, key, val) reprodyne_internal_write_indeterminate(scope, key, val)
#define reprodyne_read_indeterminate(scope, key) reprodyne_internal_write_indeterminate(scope, key)
#define reprodyne_intercept_indeterminate(scope, key, val) reprodyne_internal_intercept_indeterminate(scope, key, val)

#define reprodyne_serialize(scope, key, call) reprodyne_internal_serialize(scope, key, call)

#else

#define reprodyne_reset()

#define reprodyne_set_playback_failure_handler(handler)

#define reprodyne_mode_play() 0
#define reprodyne_mode_record() 0

#define reprodyne_record()
#define reprodyne_save(path)
#define reprodyne_play(path)

#define reprodyne_open_scope(scope)
#define reprodyne_mark_frame()

#define reprodyne_write_indeterminate(scope, key, val)
#define reprodyne_read_indeterminate(scope, key)
#define reprodyne_intercept_indeterminate(scope, key, val) val

#define reprodyne_serialize_call(scope, key, call)


#endif //REPRODYNE_AVAILABLE
#endif //REPRODYNE_H
