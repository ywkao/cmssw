#ifndef __LoadCalibrationParameters__
#define __LoadCalibrationParameters__

#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include "TString.h"

class CalibrationParameterLoader {
public:
    CalibrationParameterLoader();
    void loadParameters();

    std::map<int, double> map_pedestals;
    std::map<int, std::vector<double> > map_cm_parameters;

private:
    TString csv_file_name;
}; // CalibrationParameterLoader

#endif // __LoadCalibrationParameters__
