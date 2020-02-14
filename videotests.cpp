#include <catch2/catch.hpp>

#include <random>

#include "user-include/reprodyne.h"

#include "test-helpers.h"

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
    reprodyne_declare_upper_video_frame_size(nullptr, basicKey, 2000, 2000);
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

    reprodyne_validate_video_frame(nullptr,
                                   key,
                                   &bitbap.data[0],
                                   bitbap.width,
                                   bitbap.height,
                                   bitbap.stride);
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
    SECTION("Declare upper video frame size is no-op in playback mode")
    {
        validateHelper(bitmap1);
        readyPlay();

        reprodyne_declare_upper_video_frame_size(nullptr, basicKey, 40000, 40000);

        validateHelper(bitmap1);
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

    auto commonTests = [&]()
    {
        SECTION("Input frame stride smaller than width")
        {
            reconfigureBitmap1validate(REPRODYNE_STAT_INPUT_VIDEO_BAD_STRIDE,
                                       "Accepted invalid stride for width",
                                       [](Bitmap& bip) { bip.stride = bip.width - 1; }); //It is exactly smaller
        }
        SECTION("Input video frame larger than max declared size")
        {
            reconfigureBitmap1validate(REPRODYNE_STAT_INPUT_VIDEO_FRAME_TOO_LARGE,
                                       "Too wide for declared size",
                                       [](Bitmap& b) { ++b.width; });

            reconfigureBitmap1validate(REPRODYNE_STAT_INPUT_VIDEO_FRAME_TOO_LARGE,
                                       "Too tall for declared size",
                                       [](Bitmap& b) { ++b.height; });
        }
    };


    SECTION("Record mode")
    {
        readyRecord();

        commonTests();
    }
    SECTION("Playback mode")
    {
        readyRecord();
        validateHelper(bitmap1);
        readyPlay();

        commonTests();

        SECTION("Input video frame mismatch")
        {
            validationFail(REPRODYNE_STAT_VALIDATION_MISMATCH,
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


TEST_CASE("Video input declare frame size")
{
    //These are only of concern in record mode, you don't have to declare size in playback mode.

    reprodyne_record();
    reprodyne_mark_frame();
    reprodyne_open_scope(nullptr);


    SECTION("Didn't set frame size")
    {
        validationFail(REPRODYNE_STAT_INPUT_VIDEO_DIMENS_UNDECLARED,
                       "Reprodyne accepted validation call without a declared size... How",
                       []()
        {
            validateHelper(bitmap1);
        });
    }
    SECTION("Set frame size each frame")
    {
        reprodyne_declare_upper_video_frame_size(nullptr, basicKey, 2000, 2000);
        validateHelper(bitmap1);
        reprodyne_declare_upper_video_frame_size(nullptr, basicKey, 2000, 2000);
        validateHelper(bitmap2);
        reprodyne_declare_upper_video_frame_size(nullptr, basicKey, 2000, 2000);
        validateHelper(bitmap3);

        reprodyne_save(videoTestDataPath);

        SUCCEED();
    }
    SECTION("Inconsistent declared frame sizes")
    {
        validationFail(REPRODYNE_STAT_INPUT_VIDEO_BAD_DIMENS,
                       "Reprodyne accepted video dimensions that were different than previous declared",
                       []()
        {
            reprodyne_declare_upper_video_frame_size(nullptr, basicKey, 2000, 2000);
            validateHelper(bitmap1);
            reprodyne_declare_upper_video_frame_size(nullptr, basicKey, 2001, 2000);
        });
    }
    //"Set once but not again" is the default for the main test case.
}
