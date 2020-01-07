#pragma once

#include "indeterminate.h"
#include "callserial.h"
#include "video.h"

namespace reprodyne
{

class LiveTape
{
public:
    virtual double intercept_indeterminate();
};

class RecordTape : public LiveTape
{
public:
    double intercept_indeterminate() final
    { recordIndeterminate(); }
};

class PlayTape : public LiveTape
{
public:
    double intercept_indeterminate() final
    { return playIndeterminate(); }
};

//Don't touch the indeterminates, just string serial and video.
class RevalidateTape : public LiveTape
{
public:

};


}//reprodyne
