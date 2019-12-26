# Reprodyne
Reprodyne is an Apache 2 licensed C/C++ library with the purpose of transforming non-deterministic funtions into deterministic ones, and to serialize output from them for testing. This is done by "intercepting" indeterminate values in record mode, resupplying them in playback mode, and comparing the saved serialized outputs to the live ones.

This affords you the benefits (immediacy and simplicity) of manual testing, with the number one advantage of automated testing (namely, automation...).

The design philosophy is that, at least generally, real world data is both easier and faster to generate and save, and more useful to test against, than mocks.

Reprodyne is not meant to *replace* your current preferred test framework, but rather to augment it's capabilities.

Reprodyne was originally developed for use in end-to-end testing of a GUI framework. I present it here for anyone whom it may benefit~


# Installation
To build Reprodyne you need the following:

### Toolchain:
- CMake >= 3.12
- C++ compiler with C++17 support

### Libraries available to CMake:
- ZLIB
- Flatbuffers
- Catch2 (Optional. Required for self-tests but not for your own use)

Reprodyne follows standard CMake usage

# not so brief
Reprodyne allows you to "intercept" indeterminate values (System events, network packets, time values, etc), serialize function calls utilizing this data, and then save to "tape". In playback mode, the indeterminates are then fed back into the functions in the order that they were originally created, and the serialized calls are then compared to the saved ones to ensure that the functions are behaving as before.
