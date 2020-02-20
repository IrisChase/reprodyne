# Reprodyne
At least generally, real world data is both easier and faster to generate, and more useful to test against, than artificial test conditions written in sterile environments. Reprodyne is an Apache 2 licensed C library, written in C++, designed to capture real world test data and integrate it into automated test suites.

Reprodyne is not meant to *replace* existing test frameworks, but rather to augment their capabilities. It should be possible, at least in theory, to integrate it with any test framework.

From a high level, Reprodyne works by transforming non-deterministic funtions into deterministic functions, and saving their results for testing. "Interceptors" capture indeterminate values in record mode and resupply them in playback mode. "Validators" save values produced by the program in record mode, and compare against them in playback, signaling a playback failure if anything is different.

The Reprodyne API is defined entirely as macros, so that there are no dependencies left over in your release versions.


# Build/Install
#### Required toolchain:
- C++ compiler with C++17 support
- CMake >= 3.12

#### The following libraries must be available to CMake:
- ZLIB
- Flatbuffers
- Catch2 (Optional. Required for testing Reprodyne itself)

Reprodyne follows standard CMake usage

e.g.

--GIVE EXAMPLE--

## Running the self tests

./reprodyne_tests

## Installing the library
blah blah bla

# Usage A.K.A. not so brief
Reprodyne allows you to "intercept" indeterminate values (System events, network packets, time values, etc), serialize function calls utilizing this data, and then save to "tape". In playback mode, the indeterminates are then fed back into the functions in the order that they were originally created, and the serialized calls are then compared to the saved ones to ensure that the functions are behaving as before.



## A trivial Example
(maybe delete the above paragraph)

The following is a minimal example

(snip with comments)

### A word on linking against Reprodyne (Important)
The Reprodyne api is defined entirely as two sets of macros in the "reprodyne.h" header, switched by defining the macro "REPRODYNE_AVAILABLE". One set expands to sane defaults so that Reprodyne doesn't end up in your release binaries, and this is the default setting. The other expands to the internal Reprodyne function calls and is enabled by defining the switch.

For Reprodyne to be usable in your project, you must define the switch macro (intended to be passed to your compiler) and link against the library itself.

The header is safe to use without linking against the library as long as the switch is not defined.

## Scopes, Frames and Subkeys oh my!

Data in Reprodyne is aligned by the combination of the the current *frame*, *scope*, and *subkey*.

The frame is hard to describe god damn it.

A lot of functions in a given frame don't have to be called in order, they just all have to be called during the frame, and the frame has to return a given output. If we required every single interception to be called in the same order that it was when the test data was generated, you would lose a ton of flexibility in refactoring. So, we don't do that. Instead, each call to intercept or serialize has a scope and a subkey, which identifies a specific tape, and the reading of this tape ignores the current read status of all the other tapes. All data in a tape that is aligned to a given frame has to be read before data aligned to the next frame can be read out; attempts to read data that belongs to the previous frame triggers a playback failure.

A scope is a void pointer, meant to point to identify some resource allocated in a deterministic order. It's a bit hard to explain conceptually, so just realize it's usually something like a "this" pointer in C++, which identifies the object currently making intercept and or serialize calls. The order of allocation is important because it is used to identify the same object from multiple runs.

If you don't have a object to bind the tapes to, then you can just pass a null pointer.

open\_scope "shadows" duplicate pointers, i.e. if you call it twice with the same pointer, all the data that was written to the previous version of the scope will be preserved, but will be effectively locked from being written to anymore. This is to protect you in case you've deallocated a resource, and by a stroke of bad luck get the same pointer address for a new version of the same *kind* of resource.

Subkeys aren't strictly necessary to make reprodyne work, but it's nice to be able to declare different streams of indeterminates in a given object as idempotent; again, this affords you flexibility in refactoring.



## Custom playback failure handling and exception/longjmp safety
I can guarantee that during a playback failure, that it is safe to longjmp out of the custom handler playback handler. Resources in these code paths are carefully managed and Reprodyne will not leak\* but obviously I cannot make that guarantee for anything actually calling the playback functions. The custom playback error handler is intended for seamless integration into your test framework, not for recovery (It *is* a test failure, afterall). The fact that it doesn't leak itself is almost a token guarantee.

I can also guarantee that it's safe to throw a C++ exception out of the custom playback handler, provided that the library is ABI compatible with the executable it is being linked against, or at the very least, that the exception can safely pass through it without causing a fuss (Reprodyne won't consume your exceptions).

\*Unless I have a bad day or something

## Tips for effective use

You want to avoid causing Reprodyne to regard idempotent calls as non-idempotent, causing a call mismatch in playback mode if you happen to change the order of things or if the order is non-deterministic to begin with. This is what scopes and subkeys are for, they provide indepedent tapes, aligned to the current frame, with which to validate in any order (Again, as long as it's aligned to the current *frame*).

You want to wait until the last minute to serialize calls to give your code flexibility in change without invalidating tests. Remember, scope keys aren't for debug information per se, they're for idempotency. Don't use several sub-keys for effectively identical operations that propogate from different regions, all the serialize calls are for is to make sure that the *output* of the program is identical! It doesn't matter how we got there, just that the result is the same, and if the serialized calls rely too much on a particular code path, then you lose the flexibility to change the path to the destination.

## Ideas for integration

The laziest way is to just inject reprodyne calls into your functions as needed, to track the indeterminate values and calls.

Of course, there is nothing stopping you from using reprodyne as a backend for a mock. The mocks could be injected in either mode and provide the interception and call matching opaquely to the rest of the application.

How you choose to integrate Reprodyne is a matter of style and design philosophy. I'm generally too lazy for mocks, but you should do what you think is right for your project.


# Contributing
Contributions are welcome, but I would prefer it if you contacted me (iris@enesda.com) before beginning work on anything non-trivial. I would hate for you to waste effort on something I can't pull because it is outside of the scope of the project.

# Credits
Created and developed by Iris Chase (iris@enesda.com)

Reprodyne was originally developed for use in end-to-end testing of a GUI framework. I present it here for anyone whom it may benefit~

# License
Reprodyne is licensed under the Apache 2 license.