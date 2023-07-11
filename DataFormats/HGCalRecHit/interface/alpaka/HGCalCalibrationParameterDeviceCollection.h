#ifndef DataFormats_HGCalRecHit_interface_alpaka_HGCalCalibrationParameterDeviceCollection_h
#define DataFormats_HGCalRecHit_interface_alpaka_HGCalCalibrationParameterDeviceCollection_h

#include "DataFormats/Portable/interface/alpaka/PortableCollection.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "DataFormats/HGCalRecHit/interface/HGCalCalibrationParameterSoA.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  namespace hgcalrechit {

    // make the names from the top-level hgcalrechit namespace visible for unqualified lookup
    // inside the ALPAKA_ACCELERATOR_NAMESPACE::hgcalrechit namespace
    using namespace ::hgcalrechit;

    // SoA in device global memory
    using HGCalCalibParamDeviceCollection = PortableCollection<HGCalCalibParamSoA>;

  }  // namespace hgcalrechit

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#endif  // DataFormats_HGCalRecHit_interface_alpaka_HGCalCalibrationParameterDeviceCollection_h
