#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/SourceFactory.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/ESProducts.h"
#include "FWCore/Framework/interface/ESTransientHandle.h"
#include "FWCore/Framework/interface/EventSetupRecordIntervalFinder.h"
#include "FWCore/ParameterSet/interface/FileInPath.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"

#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ESGetToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ESProducer.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ModuleFactory.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaInterface/interface/host.h"
#include "HeterogeneousCore/AlpakaInterface/interface/memory.h"

#include "CondFormats/DataRecord/interface/HGCalCondSerializableModuleInfoRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializableModuleInfo.h"

#include "DataFormats/Math/interface/libminifloat.h"

#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterProvider.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterHostCollection.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/alpaka/HGCalCalibrationParameterDeviceCollection.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  class HGCalRecHitCalibrationESProducer : public ESProducer {
  public:

    HGCalRecHitCalibrationESProducer(edm::ParameterSet const& iConfig)
      : ESProducer(iConfig),
        filename_(iConfig.getParameter<std::string>("filename")) {

      auto cc = setWhatProduced(this);
      moduleInfoToken_ = cc.consumes();
    }

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
      edm::ParameterSetDescription desc;
      desc.add<std::string>("filename", {});
      desc.add<edm::ESInputTag>("ModuleInfo",edm::ESInputTag(""));
      descriptions.addWithDefaultLabel(desc);
    }

    typedef edm::ESProducts<std::optional<hgcalrechit::HGCalCalibParamHostCollection>, std::optional<hgcalrechit::HGCalConfigParamHostCollection>> ReturnProducts;
    ReturnProducts produce(HGCalCondSerializableModuleInfoRcd const& iRecord) {
      auto const& moduleInfo = iRecord.get(moduleInfoToken_);

      // load config
      std::tuple<uint16_t,uint8_t,uint8_t,uint8_t> denseIdxMax = moduleInfo.getMaxValuesForDenseIndex();    

      HGCalCalibrationParameterProviderConfig ccp; // config_calib_provider
      ccp.EventSLinkMax        = std::get<0>(denseIdxMax);
      ccp.sLinkCaptureBlockMax = std::get<1>(denseIdxMax);
      ccp.captureBlockECONDMax = std::get<2>(denseIdxMax);
      ccp.econdERXMax          = std::get<3>(denseIdxMax);
      ccp.erxChannelMax        = 37+2;//+2 for the two common modes

      uint32_t const size = ccp.EventSLinkMax*ccp.sLinkCaptureBlockMax*ccp.captureBlockECONDMax*ccp.econdERXMax*ccp.erxChannelMax;
      hgcalrechit::HGCalCalibParamHostCollection product(size, cms::alpakatools::host());
      // auto product = std::make_unique<hgcalrechit::HGCalCalibParamHostCollection>(size, cms::alpakatools::host());

      uint32_t const size_roc = ccp.EventSLinkMax*ccp.sLinkCaptureBlockMax*ccp.captureBlockECONDMax*ccp.econdERXMax;
      hgcalrechit::HGCalConfigParamHostCollection product_rocParam(size_roc, cms::alpakatools::host());

      product.view().config() = ccp;

      // load channel-level calib parameters
      edm::FileInPath fip(filename_);
      std::ifstream file(fip.fullPath());
      std::string line;
      uint32_t id;
      float ped,noise,cm_slope,cm_offset,bxm1_slope,bxm1_offset;
      while(std::getline(file, line)) {
        if(line.find("Channel")!=std::string::npos || line.find("#")!=std::string::npos) continue;

        std::istringstream stream(line);
        stream >> std::hex >> id >> std::dec >> ped >> noise >> cm_slope >> cm_offset >> bxm1_slope >> bxm1_offset;

        // convert electronicsId to index from denseMap
        uint32_t idx = ccp.denseMap(id);
        uint32_t rocIdx = ccp.rocDenseMap(id); // it works but we need to adapt it to contents in the yaml file

        LogDebug("HGCalCalibrationESProducer")
            << "id = " << id << ", "
            << "idx = " << idx << ", "
            << "rocIdx = " << rocIdx << ", "
            << "ped = " << ped << ", "
            << std::endl;

        // Comment: if planning to use MiniFloatConverter::float32to16(), a host function,
        // one needs to think how to perform MiniFloatConverter::float16to32() in kernels running on GPU (HGCalRecHitCalibrationAlgorithms.dev.cc)
        product.view()[idx].pedestal()    = ped;
        product.view()[idx].CM_slope()    = cm_slope;
        product.view()[idx].CM_offset()   = cm_offset;
        product.view()[idx].BXm1_slope()  = bxm1_slope;
        product.view()[idx].BXm1_offset() = bxm1_offset;

      }

      // load roc-level calib parameters
      //---------- place holder ----------//
      product_rocParam.view().config() = ccp;
      std::vector dummy_values = {7., 11., 13., 17., 19., 23.};
      for(int rocIdx=0; rocIdx<6; ++rocIdx) {
        float gain = dummy_values[rocIdx]; // rocIdx to be determined from electronics Id, maybe?
        product_rocParam.view()[rocIdx].gain() = gain;
      }
      //---------- end of place holder ----------//

      return edm::es::products(product, product_rocParam);
    } // end of produce()

  private:
    edm::ESGetToken<HGCalCondSerializableModuleInfo, HGCalCondSerializableModuleInfoRcd> moduleInfoToken_;
    HGCalCondSerializableModuleInfo moduleInfo;

    const std::string filename_;
  };

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

DEFINE_FWK_EVENTSETUP_ALPAKA_MODULE(HGCalRecHitCalibrationESProducer);
