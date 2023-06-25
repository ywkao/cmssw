#include "DQM/HGCal/interface/RunningStatistics.h"
#include "DQM/HGCal/interface/RunningCollection.h"

RunningCollection::RunningCollection()
{
    Nchannels = 234;

    for(int i=0; i<Nchannels; ++i)
    {
        RunningStatistics instance;
        mVecRunningStatistics.push_back(instance);
    }
}

RunningCollection::~RunningCollection() {}

void RunningCollection::add_entry(int channleId, double value, double cm)
{
    if(channleId>=234) {
        printf("[ERROR] RunningCollection::add_entry: the channel ID is out of scope\n");
        return;
    }

    mVecRunningStatistics[channleId].add_entry(cm, value);
}

void RunningCollection::write_csv_file() {};
