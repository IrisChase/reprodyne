# Reprodyne
Reprodyne is an Apache 2 licensed C/C++ library with the purpose of transforming non-deterministic funtions into deterministic ones, and serializing output from them for testing. This is done by "intercepting" indeterminate values in record mode, resupplying them in playback mode, and comparing the saved serialized outputs to the live ones.

The design philosophy is that, at least generally, real world data is both easier and faster to generate and save, and more useful to test against, than mocks. This affords you the benefits (immediacy and simplicity) of manual testing, with the number one advantage of automated testing (namely, automation...).

Reprodyne is not meant to *replace* your current preferred test framework, but rather to augment it's capabilities.



# Installation
## Building from source
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

# Usage A.K.A not so brief
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

dit dot dit

More detailed explanation of it.


## Custom playback failure handling and exception/longjmp safety
I can guarantee that during a playback failure, that it is safe to longjmp out of the custom handler playback handler. Resources in these code paths are carefully managed and Reprodyne will not leak\* but obviously I cannot make that guarantee for anything actually calling the playback functions. The custom playback error handler is intended for seamless integration into your test framework, not for recovery (It *is* a test failure, afterall). The fact that it doesn't leak itself is almost a token guarantee.

I can also guarantee that it's safe to throw a C++ exception out of the custom playback handler, provided that the library is ABI compatible with the executable it is being linked against, or at the very least, that the exception can safely pass through it without causing a fuss (The code paths calling the playback failure handler do not contain any try/catch blocks).

*Unless I have a bad day or something


# Contributing
Contributions are welcome, but I would prefer it if you contacted me (iris@enesda.com) before beginning work on anything non-trivial to avoid wasting time.

# Credits
Concieved and developed by Iris Chase (iris@enesda.com)

Reprodyne was originally developed for use in end-to-end testing of a GUI framework. I present it here for anyone whom it may benefit~

# License
