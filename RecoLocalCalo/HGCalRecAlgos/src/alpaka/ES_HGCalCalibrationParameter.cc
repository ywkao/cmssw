#include "HeterogeneousCore/AlpakaCore/interface/alpaka/typelookup.h"
#include "DataFormats/HGCalRecHit/interface/HGCalCalibrationParameterHostCollection.h"
#include "DataFormats/HGCalRecHit/interface/alpaka/HGCalCalibrationParameterDeviceCollection.h"

TYPELOOKUP_ALPAKA_DATA_REG(hgcalrechit::HGCalCalibParamHostCollection);
TYPELOOKUP_ALPAKA_DATA_REG(ALPAKA_ACCELERATOR_NAMESPACE::hgcalrechit::HGCalCalibParamDeviceCollection); // not working
