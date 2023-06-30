#ifndef RecoLocalCalo_HGCalRecAlgos_plugins_alpaka_HGCalRecHitCalibrationAlgorithms_h
#define RecoLocalCalo_HGCalRecAlgos_plugins_alpaka_HGCalRecHitCalibrationAlgorithms_h

#include "DataFormats/PortableTestObjects/interface/alpaka/TestDeviceCollection.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  class HGCalRecHitCalibrationAlgorithms {
  public:
    void fill(Queue& queue, portabletest::TestDeviceCollection& collection, double xvalue = 0.) const;
  };

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#endif  // RecoLocalCalo_HGCalRecAlgos_plugins_alpaka_HGCalRecHitCalibrationAlgorithms_h
