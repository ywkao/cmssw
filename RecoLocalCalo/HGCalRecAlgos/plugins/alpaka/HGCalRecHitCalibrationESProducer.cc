#include "FWCore/Framework/interface/ESTransientHandle.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ESGetToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ESProducer.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ModuleFactory.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaInterface/interface/host.h"
#include "HeterogeneousCore/AlpakaInterface/interface/memory.h"

#include "CondFormats/DataRecord/interface/HGCalCondSerializablePedestalsRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializablePedestals.h"
#include "CondFormats/DataRecord/interface/HGCalCondSerializableModuleInfoRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializableModuleInfo.h"

#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterProvider.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterESRecords.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterHostCollection.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/alpaka/HGCalCalibrationParameterDeviceCollection.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {
  /**
   * This class demonstrates and ESProducer on the data model 1 that
   * consumes a standard host ESProduct and converts the data into
   * PortableCollection, and implicitly transfers the data product to device
   */
  class HGCalRecHitCalibrationESProducer : public ESProducer {
  public:
    HGCalRecHitCalibrationESProducer(edm::ParameterSet const& iConfig) : ESProducer(iConfig) {
      auto cc = setWhatProduced(this);
      // tokenConds_ = cc.consumesFrom<HGCalCondSerializablePedestals, HGCalCondSerializablePedestalsRcd, edm::Transition::BeginRun>(edm::ESInputTag(iConfig.getParameter<std::string>("pedestal_label")));
      // moduleInfoToken_ = cc.consumesFrom<HGCalCondSerializableModuleInfo, HGCalCondSerializableModuleInfoRcd, edm::Transition::BeginRun>(edm::ESInputTag(iConfig.getParameter<edm::ESInputTag>("ModuleInfo")));

      tokenConds_ = cc.consumes<HGCalCondSerializablePedestals, HGCalCondSerializablePedestalsRcd>(edm::ESInputTag(iConfig.getParameter<std::string>("pedestal_label")));
      moduleInfoToken_ = cc.consumes<HGCalCondSerializableModuleInfo,HGCalCondSerializableModuleInfoRcd>(edm::ESInputTag(iConfig.getParameter<edm::ESInputTag>("ModuleInfo")));
    }

    // void beginRun(edm::Run const& iRun, const edm::EventSetup& iSetup) override {
    //     input = iSetup.getData(tokenConds_);
    //     moduleInfo = iSetup.getData(moduleInfoToken_);
    // }

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
      edm::ParameterSetDescription desc;
      desc.add<std::string>("pedestal_label", "");
      desc.add<edm::ESInputTag>("ModuleInfo",edm::ESInputTag(""));
      descriptions.addWithDefaultLabel(desc);
    }

    std::optional<hgcalrechit::HGCalCalibParamHostCollection> produce(HGCalCondSerializablePedestalsRcd const& iRecord) {

      std::tuple<uint16_t,uint8_t,uint8_t,uint8_t> denseIdxMax = moduleInfo.getMaxValuesForDenseIndex();    

      HGCalCalibrationParameterProviderConfig ccp; // config_calib_provider
      ccp.EventSLinkMax        = std::get<0>(denseIdxMax);
      ccp.sLinkCaptureBlockMax = std::get<1>(denseIdxMax);
      ccp.captureBlockECONDMax = std::get<2>(denseIdxMax);
      ccp.econdERXMax          = std::get<3>(denseIdxMax);
      ccp.erxChannelMax        = 37+2;//+2 for the two common modes

      uint32_t const size = ccp.EventSLinkMax*ccp.sLinkCaptureBlockMax*ccp.captureBlockECONDMax*ccp.econdERXMax*ccp.erxChannelMax;
      hgcalrechit::HGCalCalibParamHostCollection product(size, cms::alpakatools::host());

      product.view().config() = ccp;

      for(auto it : input.params_) {
        HGCalElectronicsId id(it.first);
        HGCalFloatPedestals table = input.getFloatPedestals(it.second);

        uint32_t idx = ccp.denseMap(id.raw()); 
        product.view()[idx].pedestal()   = table.pedestal;
        product.view()[idx].CM_slope()   = table.cm_slope;
        product.view()[idx].CM_offset()  = table.cm_offset;
        product.view()[idx].BXm1_kappa() = table.kappa_bxm1;
      }

      return product;
    }

  private:
    edm::ESGetToken<HGCalCondSerializablePedestals, HGCalCondSerializablePedestalsRcd> tokenConds_;
    edm::ESGetToken<HGCalCondSerializableModuleInfo, HGCalCondSerializableModuleInfoRcd> moduleInfoToken_;
    HGCalCondSerializablePedestals input;
    HGCalCondSerializableModuleInfo moduleInfo;
  };
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

DEFINE_FWK_EVENTSETUP_ALPAKA_MODULE(HGCalRecHitCalibrationESProducer);
