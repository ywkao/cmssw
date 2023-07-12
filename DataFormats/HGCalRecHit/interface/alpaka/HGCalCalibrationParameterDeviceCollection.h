#ifndef DataFormats_HGCalRecHit_interface_alpaka_HGCalCalibrationParameterDeviceCollection_h
#define DataFormats_HGCalRecHit_interface_alpaka_HGCalCalibrationParameterDeviceCollection_h

#include "DataFormats/Portable/interface/alpaka/PortableCollection.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaInterface/interface/memory.h"
#include "DataFormats/HGCalRecHit/interface/HGCalCalibrationParameterSoA.h"
#include "DataFormats/HGCalRecHit/interface/HGCalCalibrationParameterHostCollection.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  namespace hgcalrechit {

    using namespace ::hgcalrechit;
    using HGCalCalibParamDeviceCollection = PortableCollection<HGCalCalibParamSoA>;

  }  // namespace hgcalrechit

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#endif  // DataFormats_HGCalRecHit_interface_alpaka_HGCalCalibrationParameterDeviceCollection_h
