#include <catch2/catch.hpp>
#include <chrono>

#include "user-include/reprodyne.h"

double time()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

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


    const auto wanSetScope1 = {time(), time(), time(), time()};
    const auto wanSetScope2 = {time(), time(), time(), time()};

    interceptHelper(&scope1, "the-wan", wanSetScope1, true);
    reprodyne_mark_frame();
    interceptHelper(&scope2, "the-wan", wanSetScope2, true);

    reprodyne_save("reprodyne-test-data.rep");
    reprodyne_play("reprodyne-test-data.rep");

    SECTION("Valid section")
    {
        reprodyne_mark_frame();

        int rescope2;
        int rescope1;

        //The actual addresses are in the opposite order now.
        reprodyne_open_scope(&rescope1);
        reprodyne_open_scope(&rescope2);

        //This is where the magic happens~
        interceptHelper(&rescope1, "the-wan", wanSetScope1, true);
        reprodyne_mark_frame();
        interceptHelper(&rescope2, "the-wan", wanSetScope2, true);
    }
}
