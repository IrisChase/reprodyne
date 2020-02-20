#include <catch2/catch.hpp>

#include <random>

#include "user-include/reprodyne.h"

#include "oopsiewhoopsie.h"

//Was going to test scope handling but honestly I think it's well
// enough covered by the other validation tests. It's handling is generic
// after all. Sure, this means we don't have "proper" blackbox testing, but be realistic.


const auto videoTestDataPath = "reprodyne-video-test-data.rep";
const char* basicKey = "vidboi";

struct Bitmap
{
    std::vector<unsigned char> data;
    int width;
    int stride;
    int height;
};


void readyRecord()
{
    reprodyne_record();
    reprodyne_mark_frame();
    reprodyne_open_scope(nullptr);
}

void readyPlay()
{
    reprodyne_save(videoTestDataPath);
    reprodyne_play(videoTestDataPath);
    reprodyne_mark_frame();
    reprodyne_open_scope(nullptr);
}

Bitmap generateBitmap(const int width, const int height)
{
    Bitmap b = {std::vector<unsigned char>(width * height), width, width, height};
    for(auto& byte : b.data) byte = time(); //Random enough for gooberment wonk.

    return b;
}

auto bitmap1 = generateBitmap(2000, 2000);
auto bitmap2 = generateBitmap(2000, 2000);
auto bitmap3 = generateBitmap(2000, 2000);

void validateHelper(Bitmap& bitbap, const char* key = basicKey)
{
    SECTION("Test Bitmap Sanity Check")
    {
        //Just quickly make sure that each bitmap is actually unique,
        // as the tests depend heavily on that fact.
        //Not a seperate test because I want each test that depends on
        // them to fail if they're not unique.

        auto compare = [&](Bitmap& other)
        {
            if(&bitbap == &other) return;

            REQUIRE(bitbap.data.size() == other.data.size());

            auto mit = bitbap.data.begin();
            auto oit = other.data.begin();

            while(mit != bitbap.data.end() && oit != other.data.end())
            {
                REQUIRE(*mit != *oit);
                ++mit;
                ++oit;
            }
        };
    }


    //Randomize stride area? TODO
    // (ehh, that should be in a fuzz test...)

    reprodyne_validate_bitmap_hash(nullptr,
                                   key,
                                   bitbap.stride,
                                   bitbap.width,
                                   bitbap.height,
                                   &bitbap.data[0]);
}

void validationFail(const int code, const char* testFailureMsg, std::function<void()> fun)
{
    try
    {
        fun();
        FAIL(testFailureMsg);
    }
    catch(OopsieWhoopsie& e)
    {
        REQUIRE(e.code == code);
    }
}


TEST_CASE("Standard video validation")
{
    readyRecord();

    SECTION("One frame")
    {
        validateHelper(bitmap1);
        readyPlay();
        validateHelper(bitmap1);
        reprodyne_assert_complete_read();
        SUCCEED(); //SUCC SEED
    }
    SECTION("Three identical frames")
    {
        validateHelper(bitmap1);
        validateHelper(bitmap1);
        validateHelper(bitmap1);
        readyPlay();
        validateHelper(bitmap1);
        validateHelper(bitmap1);
        validateHelper(bitmap1);
        reprodyne_assert_complete_read();
        SUCCEED();
    }
    SECTION("Three distinct frames")
    {
        validateHelper(bitmap1);
        validateHelper(bitmap2);
        validateHelper(bitmap3);
        readyPlay();
        validateHelper(bitmap1);
        validateHelper(bitmap2);
        validateHelper(bitmap3);
        reprodyne_assert_complete_read();
        SUCCEED();
    }
    SECTION("Variable input frame dimensions")
    {
        auto bitmap2butsmaller = bitmap2;
        --bitmap2butsmaller.width;
        auto bitmap3butsmaller  = bitmap3;
        ----bitmap3butsmaller.width;

        validateHelper(bitmap1);
        validateHelper(bitmap2butsmaller);
        validateHelper(bitmap3butsmaller);
        readyPlay();
        validateHelper(bitmap1);
        validateHelper(bitmap2butsmaller);
        validateHelper(bitmap3butsmaller);
        reprodyne_assert_complete_read();
        SUCCEED();
    }
}


TEST_CASE("Video frame failure conditions")
{
    auto reconfigureBitmap1validate = [](const int code,
                                         const char* testFailureMsg,
                                         std::function<void(Bitmap&)> reconfigurator)
    {
        validationFail(code, testFailureMsg, [reconfigurator]()
        {
            Bitmap bcopy = bitmap1;
            reconfigurator(bcopy);
            validateHelper(bcopy);
        });
    };

    //TODO: test bad dimensions... But this is a runtime error soooooooo


    SECTION("Playback mode")
    {
        readyRecord();
        validateHelper(bitmap1);
        readyPlay();

        SECTION("Input video frame mismatch")
        {
            validationFail(REPRODYNE_STAT_CALL_MISMATCH,
                           "Accepted input was different from recorded",
                           []() { validateHelper(bitmap2); });
        }
        SECTION("Video input overflow/read past end")
        {
            validationFail(REPRODYNE_STAT_TAPE_PAST_END,
                           "Read past end successfully",
                           []()
            {
                validateHelper(bitmap1);
                validateHelper(bitmap1);
            });
        }
        SECTION("Video input underflow/incomplete read")
        {
            validationFail(REPRODYNE_STAT_PROG_TAPE_INCOMPLETE_READ,
                           "Incomplete read succesfully asserted, oops",
                           []() { reprodyne_assert_complete_read(); });
        }
        SECTION("Input frame dimensions are different")
        {
            reconfigureBitmap1validate(REPRODYNE_STAT_FRAME_MISMATCH,
                                       "Accepted frame with too small a width",
                                       [](Bitmap& bip) { --bip.width; });

            reconfigureBitmap1validate(REPRODYNE_STAT_FRAME_MISMATCH,
                                       "Accepted frame with too small a height",
                                       [](Bitmap& bip) { --bip.height; });

            //We'll just assume that if it can do either, it can do both

            //We're also not testing whether we get a mismatch when width/height > recorded.
            //This is just a judgement call and so chosen because the test helpers are done
            // with the assumption that the max size is the same as the default size of the bitmaps.
            //If we need to test this condition (Which would only fail on an absolutely assanine implementation)
            // then it should just be in it's own test case because it has to break that assumption.
        }
    }
    SECTION("Playback on empty tape")
    {
        readyRecord();
        readyPlay();

        validationFail(REPRODYNE_STAT_TAPE_PAST_END,
                       "Read worked empty tape succesfully",
                       []()
        {
            validateHelper(bitmap1);
        });
    }
}
