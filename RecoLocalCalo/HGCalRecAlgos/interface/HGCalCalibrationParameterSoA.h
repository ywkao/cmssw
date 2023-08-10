#ifndef RecoLocalCalo_HGCalRecAlgos_interface_HGCalCalibrationParameterSoA_h
#define RecoLocalCalo_HGCalRecAlgos_interface_HGCalCalibrationParameterSoA_h

#include <Eigen/Core>
#include <Eigen/Dense>

#include "DataFormats/SoATemplate/interface/SoACommon.h"
#include "DataFormats/SoATemplate/interface/SoALayout.h"
#include "DataFormats/SoATemplate/interface/SoAView.h"

#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterProvider.h"

namespace hgcalrechit {

  GENERATE_SOA_LAYOUT(HGCalCalibrationParameterSoALayout, // channel-level
                      SOA_SCALAR(HGCalCalibrationParameterProviderConfig, config),
                      SOA_COLUMN(float, pedestal),
                      SOA_COLUMN(float, CM_slope),
                      SOA_COLUMN(float, CM_offset),
                      SOA_COLUMN(float, BXm1_slope),
                      SOA_COLUMN(float, BXm1_offset)
  )
  using HGCalCalibParamSoA = HGCalCalibrationParameterSoALayout<>;
  
  GENERATE_SOA_LAYOUT(HGCalConfigParameterSoALayout, // ROC-level
                      SOA_SCALAR(HGCalCalibrationParameterProviderConfig, config),
                      SOA_COLUMN(float, gain)
  )
  using HGCalConfigParamSoA = HGCalConfigParameterSoALayout<>;
}  // namespace hgcalrechit

#endif  // RecoLocalCalo_HGCalRecAlgos_interface_HGCalCalibrationParameterSoA_h
