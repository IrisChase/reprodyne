# Reprodyne
Reprodyne is an Apache 2 licensed C/C++ library for automating manual tests.

Generally speaking, real world data is both easier and faster to generate, and more interesting to test against, than artificial test conditions written in sterile environments. Reproducibility, however, is limited. Reprodyne attempts to solve this problem by "recording" manual tests to later be played back automatically.

Reprodyne is not meant to *replace* existing test frameworks, but rather to augment their capabilities. It should be possible, at least in theory, to integrate it with any test framework.

From a high level, Reprodyne works by transforming non-deterministic funtions into deterministic ones, and saving their results for testing. "Interceptors" capture indeterminate values in record mode and resupply them in playback mode. "Validators" save values produced by the program in record mode, and compares against them in playback mode, signaling a playback failure if anything is different.

The Reprodyne API is defined entirely as a set of preprocessor macros, so that once you're done testing, they gracefully expand into no-ops and there is no longer a need to link against the library.

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

---
**Note**: If you want to explore the code for the library itself, you will probably want to build it first as the source calls generated header code and your tools might get mad at you if it doesn't exist yet.


## Running The Library Tests

Simply run the "reprodyne_tests" executable located in the build directory:

    ./reprodyne_tests

## Including Reprodyne in Your Projects
### CMake Examples

    #Exact match for major version 1, and minimum minor/patch verion 0
    find_package(reprodyne 1.0.0 REQUIRED)

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

If you're not using CMake do whatever you have to do to point your non-CMake peasant-build system to the directory containing the appropriate version of this header.

# How it Works/Theory

As far as I can see, there are two ways to use Reprodyne. You can either integrate the interceptors and validators directly into your codebase (Which has first-class support, that is why the macros can compile into no-ops), or you can write mocks that use them behind the scenes. I will not dictate what the "correct" approach is, as this is a decision best left to the particular developer(s) of a given project.



## Scopes, Frames and Subscope Keys oh my!

Data in Reprodyne is aligned by the combination of the the current *frame*, *scope*, and *subscope*.

Playback data is bound to a *scope*, which is represented by a pointer. The idea is that for a given scope you have a set of indeterminates going in, and values coming out, independent of the order of execution of any other scope currently being executed. This reduces coupling between the tests and the absolute order of execution.

Frames are the synchronization mechanism for scopes. The intention being that for a given go-round of the program's *main outer loop*, all scopes should be done processing whatever it is that they are responsible for, *for a given iteration*. Again, frames are *only* to be marked at a high level of the program.

Subscopes follow the same rules as scopes except: They are addressed by a their parent scope and a string key. A given scope (Which is most commonly just an object) might have several things going on where the order of execution is not *inherently* serial. In this case, we don't want Reprodyne to enforce the order of execution, otherwise it would limit our ability to refactor. To solve this problem, the order of execution is only tracked on the subscope level. This prevents tight coupling of the tests to arbitrary design decisions that don't affect the result we care about. One nice side-benefit of this is that naming the subscope allows Reprodyne to tell you exactly what part of the scope failed and to a limited extent, how.

You may think that an object should only do one thing, and that anything else should be handled by other objects. Child objects, perhaps. This was actually the driving motivion *for* subscopes. Working this way, you simply register the scope of a primary object, and then any child objects use subscopes, this way the *hierarchy* of excution is preserved but not the order of the subscopes. You could track the addresses of all these child objects, but that could become unwieldy, it's hard enough to guarantee the order of allocation of a bunch of root objects, much less all of their children, grandchildren, etc...

## Interceptors and Validators

In record mode, an intercept function takes a value, saves it against the scope/subscope pair, along with the frame ID, and then returns it to your code like nothing ever happened.

In playback mode, it is exactly the same *interface,* but the intercepted value is discarded and Reprodyne attempts to retrieve one saved previously to be returned so your code runs exactly as it did in record mode.

Care must be taken to ensure that you intercept all *true* indeterminate values. Otherwise you'll get "almost deterministic" behaviour.

If anything is amiss with the number of calls to a given scope, or there is a difference in frame ID or the like, then a playback error will be raised.

Validators are just like interceptors, but they return nothing, and in playback mode they attempt to compare the provided value against the stored one.

Errors are likewise the same for validators but they will additionally raise a playback error if the stored value is different from the provided one.

Currently, interceptors can intercept doubles and validators can validate strings and hashes of bitmaps. This will be extended in the future.

# Learn by Example

There are two classes of functions in Reprodyne. Functions intended to be called by the test administration code, and functions intended to be called within the code under test.

Your code under test might look something like this:

    #include <iostream>
    #include <reprodyne.h>
    #include "someruntimeclass.h"
    #include "somesortofsystemeventheader.h"

    void processSomething()
    {
        SomeRuntimeClass rt;
        
        while(rt.yupStillRunning())
        {
            reprodyne_mark_frame();

            const auto eventCode = reprodyne_intercept_double(&rt, "System Event", getSystemEvent());

            rt.reactToSystemEvent(eventCode);

            std::cout << rt.resultString() << std::endl;

            reprodyne_validate_string(&rt, "Processing Result", rt.resultString());
        }
    }

---

There are four states this code could be under when executed:

- Reprodyne is not compiled in, and the calls are no-ops.
- You're in record mode and the values are being captured, a test failure is not possible, this is where you test the executable manually to verify that it is functioning as intended.
- You're in playback mode and the values are being intercepted/validated, a playback failure will be raised if there is an issue.
- Reprodyne is compiled in but the setup code hasn't been called by the test setup code. This will fail.


For record mode, you probably want a custom executable for generating the test data, where the main function might look something like this:

    #include <reprodyne.h>

    void processSomething();

    int main(int argc, char** argv)
    {
        reprodyne_record();

        processSomething();

        reprodyne_save("PATH-TO-SAVE-TEST-DATA.x3th");

        return 0;
    }

**NOTE:** The file extension is somewhat arbitrary, Reprodyne is able to recognize one of it's own.

---

Then to execute the test:

    void test()
    {
        reprodyne_play("PATH-TO-SAVED-TEST-DATA.rep");

        processSomething();

        reprodyne_assert_complete_read(); //Make sure the process didn't exit early.
    }

---

### Making Reprodyne Available to Your Code

Reprodyne macros expand to no-ops by default. In order to use Reprodyne you must define the following:

    REPRODYNE_AVAILABLE

It is recommended that this be defined by your build system/compiler, as it needs to be defined everywhere Reprodyne is used, not just in your test code but your code under test.

## Gotchas

reprodyne_open_scope tracks *the temporal position of objects.* That is, Reprodyne saves the *order in which the scopes are opened.* The actual values of the pointers is irrelevant. The only requirement is that scopes *must* be opened in the same order every time.

One pernicious edge-case that may not seem obvious is that it is possible for pointers to be re-used for different objects (E.g. a memory pool). If a pointer is already tracked during a call to reprodyne_open_scope, Reprodyne will *shadow* it with a new scope. It is possible that a pointer is reused in one run of the application and not another, or in record mode but not in subsequent playbacks. Again, this is fine because the scopes only track the *temporal location* of the pointers and the actual address is irrelevant. They could all be null pointers or arbitrary integers so long as they *represent* one and only one scope at a time and that those scopes are always represented in the same order.

You must call mark frame at least once, even if you have a case where the "frame" model doesn't make sense.

## Custom Playback Failure Handling and Exception/Longjmp Safety

It may be convienient to provide a custom playback failure handler to help weave Reprodyne into whatever test framework/ungodly mess you are dealing with. But it's not recommended unless you have to. Reprodyne by default prints a message to stderr and then aborts the executable, which most test frameworks should be able to handle out of the box. But read on if you must...

It is safe to longjmp out of the custom playback failure handler. The only hard rule is that it cannot simply return, if it tries, the application will then terminate.

Exceptions... Well, they work on my machine.

Reprodyne will not catch any user exceptions from that handler and the code path for the error handler is written to be longjmp safe, so of course it won't leak given an exception, but I am in no position to guarantee whether or not an exception from a binary with an incompatible exception model will pass through without causing a fuss; it might work, it might not.

If you run into issues with C++ exceptions, I'd recommend just using setjmp/longjmp if you must retrieve control in this way.

# Reprodyne is no Testing Panacea

Reprodyne is not a replacement for traditional testing, it's an alternative method for when it makes sense to do things differently.

If you don't play your cards right, it can be very easy to break your tests without invalidating your code. Although... That happens with traditional testing as well, just perhaps not as easily.

An example of a false positive test failure would be if you manage to remove extraneous iterations from your main loop, invalidating the playback data. Even for a single frame this will invalidate everything. The closer you can map your calls to Reprodyne to the semantic, unchangeable meaning of your operations, the better. But this isn't necessarily easy.

I've tried to make it as flexible as possible and in the future with more experience using it, I hope to make it more so. For now at least, I feel as though the benefits of Reprodyne already outweigh these concerns in certain situations.


# Reference Documentation and Getting Help

For reference, the reprodyne.h header documents all of the interface calls and is a short read. If you have any questions after that, feel free to open an issue in the tracker with the "question" label.


# Contributing
Contributions are welcome, but please open an issue with the "Proposal" label in the issue tracker to begin discussion before beginning work on anything non-trivial. I would hate for you to waste effort on something I can't pull because it's not how I want to proceed.

Alternatively, you can contact me directly (iris@enesda.com) and I'll manage the issue for you.

# Credits
Created and developed by Iris Chase (iris@enesda.com)

Reprodyne was originally developed for use in end-to-end testing of a GUI framework. I present it here for anyone whom it may benefit~

# License
Reprodyne is licensed under the Apache 2.0 license.