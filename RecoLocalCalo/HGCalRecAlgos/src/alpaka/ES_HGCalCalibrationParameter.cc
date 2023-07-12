#include "HeterogeneousCore/AlpakaCore/interface/alpaka/typelookup.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterHostCollection.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/alpaka/HGCalCalibrationParameterDeviceCollection.h"

TYPELOOKUP_ALPAKA_DATA_REG(hgcalrechit::HGCalCalibParamHostCollection);
TYPELOOKUP_ALPAKA_DATA_REG(ALPAKA_ACCELERATOR_NAMESPACE::hgcalrechit::HGCalCalibParamDeviceCollection); // not working
