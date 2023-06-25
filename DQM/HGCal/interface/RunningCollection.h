#ifndef __RUNNING_COLLECTION__
#define __RUNNING_COLLECTION__
#include <vector>
#include "RunningStatistics.h"

class RunningCollection{
    public:
        RunningCollection();
        ~RunningCollection();

        void add_entry(int channleId, double value, double cm);
        std::vector<RunningStatistics> get_vector_running_statistics() {return mVecRunningStatistics;};
        void write_csv_file();

    private:
        int Nchannels;
        std::vector<RunningStatistics> mVecRunningStatistics;

};

#endif
