#ifndef RecoLocalCalo_HGCalRecAlgos_interface_HGCalCalibrationParameterSoA_h
#define RecoLocalCalo_HGCalRecAlgos_interface_HGCalCalibrationParameterSoA_h

#include <Eigen/Core>
#include <Eigen/Dense>

#include "DataFormats/SoATemplate/interface/SoACommon.h"
#include "DataFormats/SoATemplate/interface/SoALayout.h"
#include "DataFormats/SoATemplate/interface/SoAView.h"

#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterProvider.h"

namespace hgcalrechit {

  GENERATE_SOA_LAYOUT(HGCalCalibrationParameterSoALayout,
                      SOA_SCALAR(HGCalCalibrationParameterProviderConfig, config),
                      SOA_COLUMN(float, gain),       // ROC-level
                      SOA_COLUMN(float, pedestal),   // channel-level
                      SOA_COLUMN(float, CM_slope),   // channel-level
                      SOA_COLUMN(float, CM_offset),  // channel-level
                      SOA_COLUMN(float, BXm1_slope), // channel-level
                      SOA_COLUMN(float, BXm1_offset) // channel-level
  )
  using HGCalCalibParamSoA = HGCalCalibrationParameterSoALayout<>;
  
}  // namespace hgcalrechit

#endif  // RecoLocalCalo_HGCalRecAlgos_interface_HGCalCalibrationParameterSoA_h
