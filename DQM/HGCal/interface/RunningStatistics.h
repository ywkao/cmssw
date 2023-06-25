#ifndef __RUNNING_STATISTICS__
#define __RUNNING_STATISTICS__

#include <stdio.h>
#include <cmath>

struct statistics {
    double nevents;
    double mean_x;
    double mean_y;
    double variance_x;
    double variance_y;
    double covariance;
    double correlation;
    double slope;
    double intercept;
};

class RunningStatistics {
    public:
        RunningStatistics();
        ~RunningStatistics(){};

        void add_entry(double x, double y);
        void print_statistics();
        double get_correlation();
        double get_slope();
        double get_intercept();
        double get_mean_adc()    { return current.mean_y;      }

    private:
        statistics current;
        statistics next;
};

#endif
