#include <catch2/catch.hpp>
#include <chrono>

#include "user-include/reprodyne.h"


class OopsieWhoopsie : public std::exception
{
public:
    OopsieWhoopsie(const int code): code(code) {}
    const int code;
};

void custom_failure_handler(const int code, const char*)
{
    throw OopsieWhoopsie(code);
}


double time()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::vector<double> generateList()
{
    //Tried this in a lambda but the compiler is too clever for it's own good.
    return std::initializer_list<double>({time(), time(), time(), time()});
};

TEST_CASE("woof")
{
    reprodyne_record();

    reprodyne_mark_frame();

    int scope1; //Only used for their addresses
    int scope2;

    reprodyne_open_scope(&scope1);
    reprodyne_open_scope(&scope2);


    auto interceptHelper = [&](void* scope, std::string key, auto list, const bool correctMode)
    {
        for(auto i : list)
        {
            const double rep = reprodyne_intercept_indeterminate(scope, key.c_str(), i);

            if(correctMode) REQUIRE(rep == i);
            else            REQUIRE(rep != i);
        }
    };


    const auto originalSetScope1 = generateList();
    const auto originalSetScope2 = generateList();

    interceptHelper(&scope1, "the-wan", originalSetScope1, true);
    reprodyne_mark_frame();
    interceptHelper(&scope2, "the-wan", originalSetScope2, true);

    reprodyne_save("reprodyne-test-data.rep");
    reprodyne_play("reprodyne-test-data.rep");

    SECTION("Correct interception")
    {
        reprodyne_mark_frame();

        int rescope2;
        int rescope1;


        //The actual addresses are in the opposite order now, but
        // as long as they are registered to reprodyne in the same order, it's fine.
        reprodyne_open_scope(&rescope1);
        reprodyne_open_scope(&rescope2);

        SECTION("Intercept supplies original indeterminates")
        {
            interceptHelper(&rescope1, "the-wan", originalSetScope1, true);
            reprodyne_mark_frame();
            interceptHelper(&rescope2, "the-wan", originalSetScope2, true);
        }
        SECTION("Discard supplied indeterminates")
        {
            //These new values are intercepted and replaced
            const auto secondSetScope1 = generateList();
            const auto secondSetScope2 = generateList();

            interceptHelper(&rescope1, "the-wan", secondSetScope1, false);
            reprodyne_mark_frame();
            interceptHelper(&rescope2, "the-wan", secondSetScope2, false);
        }
    }
    SECTION("Mismatched frame")
    {
        reprodyne_mark_frame();

        int rescope2;
        int rescope1;

        //The actual addresses are in the opposite order now.
        reprodyne_open_scope(&rescope1);
        reprodyne_open_scope(&rescope2);

        //This is where the magic happens~
        interceptHelper(&rescope1, "the-wan", originalSetScope1, true);
        /*MARK FRAME MISSING*/

        reprodyne_set_playback_failure_handler(&custom_failure_handler);

        bool success = false;

        try
        {
            interceptHelper(&rescope2, "the-wan", originalSetScope2, false); //This call should fail
        }
        catch(const OopsieWhoopsie oops)
        {
            REQUIRE(oops.code == REPRODYNE_STAT_FRAME_MISMATCH);
            success = true;
        }

        REQUIRE(success);
    }
}
