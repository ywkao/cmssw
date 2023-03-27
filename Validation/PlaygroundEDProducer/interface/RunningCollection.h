#ifndef __RUNNING_COLLECTION__
#define __RUNNING_COLLECTION__
#include <vector>
#include "RunningStatistics.h"

class RunningCollection {
public:
    RunningCollection() {
        Nchannels = 234;
        for(int i=0; i<Nchannels; ++i) {
            RunningStatistics instance;
            mVecRunningStatistics.push_back(instance);
        }
    }

    void add_entry(int channleId, double value, double cm) {
        if(channleId>=234) {
            printf("[ERROR] RunningCollection::add_entry: the channel ID is out of scope\n");
            return;
        }
        mVecRunningStatistics[channleId].add_entry(cm, value);
    }

    std::vector<RunningStatistics> get_vector_running_statistics() {return mVecRunningStatistics;};

    void write_csv_file() {};

private:
    int Nchannels;
    std::vector<RunningStatistics> mVecRunningStatistics;
};

#endif
