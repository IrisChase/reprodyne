#include <catch2/catch.hpp>
#include <chrono>

#include "user-include/reprodyne.h"


class OopsieWhoopsie : public std::exception
{
public:
    OopsieWhoopsie(const int code): code(code) {}
    const int code;
};

void code_gobbling_error_handler(const int code, const char*)
{
    throw OopsieWhoopsie(code);
}

double time()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::vector<double> generateList()
{
    //Tried this in a lambda but gcc is too clever for it's own good.
    return std::initializer_list<double>({time(), time(), time(), time()});
}

TEST_CASE("woof")
{
    reprodyne_record();
    reprodyne_mark_frame();

    int scope1; //Only used for their addresses
    int scope2;

    reprodyne_open_scope(&scope1);
    reprodyne_open_scope(&scope2);


    auto interceptHelper = [&](void* scope, std::string key, auto list, std::vector<double> validationSet)
    {
        for(int i = 0; i != list.size(); ++i)
        {
            const double rep = reprodyne_intercept_indeterminate(scope, key.c_str(), list[i]);

            if(validationSet.size()) REQUIRE(validationSet[i] == rep);
        }
    };

    auto callHelper = [&](void* scope, std::string key, auto list)
    {
        for(const double i : list)
            reprodyne_serialize(scope, key.c_str(), std::string("pretendFunctionCall(" + std::to_string(i) + ");").c_str());
    };


    const auto originalSetScope1 = generateList();
    const auto originalSetScope2 = generateList();

    //Consequently, this tests that the original values are returned
    interceptHelper(&scope1, "the-wan", originalSetScope1, originalSetScope1);
    reprodyne_mark_frame();
    interceptHelper(&scope2, "the-wan", originalSetScope2, originalSetScope2);
    callHelper(&scope2, "the-wan", originalSetScope2);

    {
        const auto testDataPath = "reprodyne-test-data.rep";
        reprodyne_save(testDataPath);
        reprodyne_play(testDataPath); //Automatically re-initializes everything, don't worry~

        reprodyne_set_playback_failure_handler(&code_gobbling_error_handler);

        reprodyne_mark_frame();
    }

    SECTION("Correct interception")
    {
        int rescope2;
        int rescope1;

        //The actual addresses are in the opposite order now, but
        // as long as they are registered to reprodyne in the same order, it's fine.
        reprodyne_open_scope(&rescope1);
        reprodyne_open_scope(&rescope2);

        SECTION("Discard supplied indeterminates, returning originals")
        {
            //These new values are intercepted and replaced
            const auto secondSetScope1 = generateList();
            const auto secondSetScope2 = generateList();

            interceptHelper(&rescope1, "the-wan", secondSetScope1, originalSetScope1);
            reprodyne_mark_frame();
            interceptHelper(&rescope2, "the-wan", secondSetScope2, originalSetScope2);
            callHelper(&rescope2, "the-wan", originalSetScope2);

            reprodyne_assert_complete_read();
        }
    }
    SECTION("Mismatched frame")
    {
        reprodyne_open_scope(&scope1);
        reprodyne_open_scope(&scope2);

        interceptHelper(&scope1, "the-wan", originalSetScope1, {});

        /*MARK FRAME MISSING*/

        bool success = false;

        try
        {
            interceptHelper(&scope2, "the-wan", originalSetScope2, {});
            FAIL("Intercept didn't fail with mismatched frame calls");
        }
        catch(const OopsieWhoopsie oops)
        {
            REQUIRE(oops.code == REPRODYNE_STAT_FRAME_MISMATCH);
            success = true;
        }

        REQUIRE(success); //, failure is NOT an option.
    }
    SECTION("Read past the end")
    {

    }
    SECTION("Call mismatch")
    {

    }
    SECTION("Unregistered scope")
    {

    }
    SECTION("Scope override")
    {

    }
    SECTION("Two subscopes, one ordinal scope")
    {

    }
    SECTION("Read without corresponding write")
    {

    }

    SECTION("Graceful handling of generally dirty inputs")
    {

    }
    SECTION("Incomplete program read")
    {

    }
    SECTION("Incomplete call read")
    {
        reprodyne_open_scope(&scope1);
        reprodyne_open_scope(&scope2);

        interceptHelper(&scope1, "the-wan", originalSetScope1, {});
        reprodyne_mark_frame();
        interceptHelper(&scope2, "the-wan", originalSetScope2, {});

        /*SERIALIZE CALL MISSING*/

        try
        {
            reprodyne_assert_complete_read();
            FAIL("Reprodyne should have aborted on incomplete call read.");
        }
        catch(const OopsieWhoopsie oops)
        {
            REQUIRE(oops.code == REPRODYNE_STAT_CALL_TAPE_INCOMPLETE_READ);
        }
    }
}
