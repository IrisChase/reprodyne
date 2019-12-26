# Reprodyne
Reprodyne is an Apache 2 licensed C/C++ library with the purpose of transforming non-deterministic funtions into deterministic ones, and to serialize output from them for testing. This is done by "intercepting" indeterminate values in record mode, resupplying them in playback mode, and comparing the saved serialized outputs to the live ones.

This affords you the benefits (immediacy and simplicity) of manual testing, with the number one advantage of automated testing (namely, automation...).

The design philosophy is that, at least generally, real world data is both easier and faster to generate and save, and more useful to test against, than mocks.

Reprodyne is not meant to *replace* your current preferred test framework, but rather to augment it's capabilities.

Reprodyne was originally developed for use in end-to-end testing of a GUI framework. I present it here for anyone whom it may benefit~


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

## Installing the libraries

### A word on compiling/linking against Reprodyne (Important)
For Reprodyne to be usable in your project, you must define "REPRODYNE_AVAILABLE" (Preferrably passed to your compiler) and link against the library itself.

The header is safe to use without linking against the library as long as "REPRODYNE_AVAILABLE" is not defined. The macros expand into sane defaults.

The Reprodyne api is defined entirely as two sets of macros in the "reprodyne.h" header, switched by defining a "REPRODYNE_AVAILABLE". One set expands to nothing so that Reprodyne doesn't end up in your release binaries, and this is the default setting. The other expands to the internal Reprodyne function calls and is enabled by defining "REPRODYNE_AVAILABLE".


# Usage A.K.A not so brief
Reprodyne allows you to "intercept" indeterminate values (System events, network packets, time values, etc), serialize function calls utilizing this data, and then save to "tape". In playback mode, the indeterminates are then fed back into the functions in the order that they were originally created, and the serialized calls are then compared to the saved ones to ensure that the functions are behaving as before.

## A trivial Example
(maybe delete the above paragraph)

The following is a minimal example

(snip with comments)



## Scopes, Frames and Subkeys oh my!

dit dot dit

More detailed explanation of it.


## Exception/longjmp safety

blah blah fucking blah.


# Contributing

# Credits

# License
