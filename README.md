# Reprodyne
Reprodyne is an Apache 2 licensed C/C++ library designed to automate manual tests.

Generally speaking, real world data is both easier and faster to generate, and more useful to test against, than artificial test conditions written in sterile environments. Reproducibility, however, is limited. Reprodyne is designed to "record" manual tests to later be played back automatically.

Reprodyne is not meant to *replace* existing test frameworks, but rather to augment their capabilities. It should be possible, at least in theory, to integrate it with any test framework.

From a high level, Reprodyne works by transforming non-deterministic funtions into deterministic ones, and saving their results for testing. "Interceptors" capture indeterminate values in record mode and resupply them in playback mode. "Validators" save values produced by the program in record mode, and compares against them in playback mode, signaling a playback failure if anything is different.

The Reprodyne API is defined entirely as a set of preprocessor macros, so that once you're done testing, they gracefully expand into no-ops and there is no longer a need to link against the library.

# Imagine Reprodyne/Use Cases

blah blah blah


# Build/Install
#### Required toolchain:
- C++ compiler with C++17 support
- CMake >= 3.12

#### The following libraries must be available to CMake:
- ZLIB
- Flatbuffers
- OpenSSL/Crypto
- Catch2 (Optional. Required for testing Reprodyne itself)

Reprodyne follows standard CMake usage e.g. from your build directory:

    cmake REPRODYNE-SOURCE-DIR
    cmake --build .
    sudo cmake --install .

Reprodyne will be installed in the following directories:

    /usr/local/lib
    /usr/local/include

You can optionally provide a prefix for the install path:

    cmake --install . --prefix THE-DIRECTORY-YOU-WANT

Which will of course prefix the install path as such:

    THE-DIRECTORY-YOU-WANT/usr/local/lib
    THE-DIRECTORY-YOU-WANT/usr/local/include

Or you can specify the lib/include paths directly:

    IRIS YOU GOTTA ADD THIS TO THE CMAKE FILE

---
**Note**: If you want to explore the code for the library itself, you will probably want to build it first as the source calls generated header code and your tools might get mad at you if it doesn't exist yet.


## Running The Library Tests

Simply run the "reprodyne_tests" executable located in the build directory:

    ./reprodyne_tests

## Including Reprodyne in Your Projects
---
### CMake Examples

    find_package(reprodyne 1.0 REQUIRED)
    target_link_library(YOUR-TARGET reprodyne)

If it is found then the include director(ies) will be in REPRODYNE_INCLUDE_DIRS:

    target_include_directories(YOUR-TARGET ${REPRODYNE_INCLUDE_DIRS})

To toggle usage of the library macros, define REPRODYNE_AVAILABLE:

    target_compile_definitions(YOUR-TARGET PRIVATE REPRODYNE_AVAILABLE)

---
### And for The Love of God

Please don't do this:

    <reprodyne1.1/reprodyne.h>

Minor versions should be backwards compatible, and with this you are hardcoding an exact minor version requirement.

With or without CMake, you should be including reprodyne as such:

    <reprodyne.h>

If you're not using CMake do whatever you have to do to point your non-CMake peasant-build system to the directory containing this header.

# Usage/Theory

As far as I can see, there are two ways to use Reprodyne. You can either integrate the interceptors and validators directly into your codebase (Which has first-class support, that is why the macros can compile into no-ops), or you can write mocks that use them behind the scenes. I will not dictate what the "correct" approach is, as this is a decision best left to the particular developer(s) of a given project.

Reprodyne itself is quite simple, but you must first understand the theory behind it, as it is designed around some not-so-obvious-at-first-glance problems.


## Scopes, Frames and Subscope keys OH MAI!

Data in Reprodyne is aligned by the combination of the the current *frame*, *scope*, and *subscope*.

Playback data is bound to a *scope*, which is represented by a pointer. The idea is that for a given scope you have a set of indeterminates going in, and values coming out, independent of the order of execution of any other scope currently being executed. This reduces coupling between the tests and the absolute order of execution.

Frames are the synchronization mechanism for scopes. The intention being that for a given go-round of the program's *main outer loop*, all scopes should be done processing whatever it is that they are responsible for, *for a given iteration*. Again, frames are *only* to be marked at a high level of the program.

Subscopes follow the same rules as scopes except: They are addressed by a their parent scope and a string key. A given scope (Which is most commonly just an object) might have several things going on where the order of execution is not *inherently* serial. In this case, we don't want Reprodyne to enforce the order of execution, otherwise it would limit our ability to refactor. To solve this problem, the order of execution is only tracked on the subscope level. This prevents tight coupling of the tests to arbitrary design decisions that don't affect the result we care about. One nice side-benefit of this is that naming the subscope allows Reprodyne to tell you exactly what part of the scope failed and to a limited extent, how.

You may think that an object should only do one thing, and that anything else should be handled by other objects. Child objects, perhaps. This was actually the driving motivion *for* subscopes. Working this way, you simply register the scope of a primary object, and then any child objects use subscopes, this way the *hierarchy* of excution is preserved but not the order of the subscopes. You could track the addresses of all these child objects, but that could become unwieldy, it's hard enough to guarantee the order of allocation of a bunch of root objects, much less all of their children, grandchildren, etc...

## Interceptors and Validators
## A trivial Example
(maybe delete the above paragraph)

The following is a minimal example

(snip with comments)

### Making Reprodyne Available to Your Code

Reprodyne is like assert in that it is defined by macros and to be compiled away when it is no longer needed. But unlike assert, the Reprodyne macros expand to no-ops by default. In order to use Reprodyne you must define the following:

    REPRODYNE_AVAILABLE

This is intended to be provided by your build system to switch between builds easily.

## Potentially Painful Gotchas

reprodyne_open_scope tracks *the temporal position of objects.* That is, Reprodyne saves the *order in which the scopes are opened.* The actual values of the pointers is irrelevant. The only requirement is that scopes *must* be opened in the same order every time.

One pernicious edge-case that may not seem obvious is that it is possible for pointers to be re-used for different objects (E.g. a memory pool). If a pointer is already tracked during a call to reprodyne_open_scope, Reprodyne will *shadow* it with a new scope. It is possible that a pointer is reused in one run of the application and not another, or in record mode but not in subsequent playbacks. Again, this is fine because the scopes only track the *temporal location* of the pointers and the actual address is irrelevant. They could all be null pointers or arbitrary integers so long as they *represent* one and only one scope at a time and in the same order.

You must call mark frame at least once, even if you have a case where the "frame" model doesn't make sense.



## Custom playback failure handling and exception/longjmp safety
I can guarantee that during a playback failure, that it is safe to longjmp out of the custom handler playback handler. Resources in these code paths are carefully managed and Reprodyne will not leak\* but obviously I cannot make that guarantee for anything actually calling the playback functions. The custom playback error handler is intended for seamless integration into your test framework, not for recovery (It *is* a test failure, afterall). The fact that it doesn't leak itself is almost a token guarantee.

I can also guarantee that it's safe to throw a C++ exception out of the custom playback handler, provided that the library is ABI compatible with the executable it is being linked against, or at the very least, that the exception can safely pass through it without causing a fuss (Reprodyne won't consume your exceptions).

\*Unless I have a bad day or something

## Reference Documentation and Getting Help.

For reference, the reprodyne.h header documents all of the interface calls and is a short read. If you have any questions after that, in lieu of emailing me, please consider opening it as a bug in the tracker so that others with your question can benefit from the answer.


# Contributing
Contributions are welcome, but I would prefer it if you contacted me (iris@enesda.com) before beginning work on anything non-trivial. I would hate for you to waste effort on something I can't pull because it's not how I want to proceed.

# Credits
Created and developed by Iris Chase (iris@enesda.com)

Reprodyne was originally developed for use in end-to-end testing of a GUI framework. I present it here for anyone whom it may benefit~

# License
Reprodyne is licensed under the Apache 2.0 license.