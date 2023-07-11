#ifndef DataFormats_HGCalRecHit_interface_HGCalCalibrationParameterHostCollection_h
#define DataFormats_HGCalRecHit_interface_HGCalCalibrationParameterHostCollection_h

#include "DataFormats/Portable/interface/PortableHostCollection.h"
#include "DataFormats/HGCalRecHit/interface/HGCalCalibrationParameterSoA.h"

namespace hgcalrechit {

  // SoA with pedestal, CM_slope, CM_offset, BXm1_kappa in host memory
  using HGCalCalibParamHostCollection = PortableHostCollection<HGCalCalibParamSoA>;

}  // namespace hgcalrechit

#endif  // DataFormats_HGCalRecHit_interface_HGCalCalibrationParameterHostCollection_h
