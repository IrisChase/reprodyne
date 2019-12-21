#ifndef REPRODYNE_H
#define REPRODYNE_H

#define REPRODYNE_AVAILABLE

#ifdef REPRODYNE_AVAILABLE

extern "C"
{

int reprodyne_internal_mode_play();
int reprodyne_internal_mode_record();

void reprodyne_internal_write_indeterminate(void* scope, const char* key, double val);
double reprodyne_internal_read_indeterminate(void* scope, const char* key);

double reprodyne_internal_intercept_indeterminate(void* scope, const char* key, double val);

} //extern "C"

//If you believe lower case macros are evil... I'm so sorry.
//Think of them like "assert" in assert.h... They're only like this to compile out of release
//I just... I can't yell this much...


#define reprodyne_mode_play() reprodyne_internal_mode_play()
#define reprodyne_mode_record() reprodyne_internal_mode_record()

#define reprodyne_record()
#define reprodyne_save(path)
#define reprodyne_play(path)

#define reprodyne_open_scope(scope)
#define reprodyne_mark_frame()

#define reprodyne_write_indeterminate(scope, key, val) reprodyne_internal_write_indeterminate(scope, key, val)
#define reprodyne_read_indeterminate(scope, key)
#define reprodyne_intercept_indeterminate(scope, key, val)

#define reprodyne_serialize_call(scope, key, call)

#else

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


#endif


#endif //REPRODYNE_H
