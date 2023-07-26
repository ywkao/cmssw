// CMSSW includes
#include "DataFormats/HGCalDigi/interface/HGCalDigiHostCollection.h"
#include "DataFormats/HGCalDigi/interface/alpaka/HGCalDigiDeviceCollection.h"
#include "DataFormats/HGCalRecHit/interface/HGCalRecHitHostCollection.h"
#include "DataFormats/HGCalRecHit/interface/alpaka/HGCalRecHitDeviceCollection.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDPutToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/Event.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/stream/EDProducer.h"
#include "HeterogeneousCore/AlpakaInterface/interface/CopyToDevice.h"
#include "RecoLocalCalo/HGCalRecAlgos/plugins/alpaka/HGCalRecHitCalibrationAlgorithms.h"

// includes for loading pedestal txt file
#include <iomanip> // for std::setw
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESWatcher.h"
#include "CondFormats/DataRecord/interface/HGCalCondSerializablePedestalsRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializablePedestals.h"
#include "CondFormats/DataRecord/interface/HGCalCondSerializableConfigRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializableConfig.h"

// include for save calibration parameter
// #include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalCalibrationParameterProvider.h"

// includes for size parameters
#include "CondFormats/DataRecord/interface/HGCalCondSerializableModuleInfoRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializableModuleInfo.h"

// include for debug info
#include "FWCore/MessageLogger/interface/MessageLogger.h"


#include <future>

template<class T> double duration(T t0,T t1)
{
  auto elapsed_secs = t1-t0;
  typedef std::chrono::duration<float> float_seconds;
  auto secs = std::chrono::duration_cast<float_seconds>(elapsed_secs);
  return secs.count();
}

inline std::chrono::time_point<std::chrono::steady_clock> now()
{
  return std::chrono::steady_clock::now();
}

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  using namespace cms::alpakatools;

  class HGCalRecHitProducer : public stream::EDProducer<> {
  public:
    explicit HGCalRecHitProducer(const edm::ParameterSet&);
    static void fillDescriptions(edm::ConfigurationDescriptions&);

  private:
    void produce(device::Event&, device::EventSetup const&) override;
    void beginRun(edm::Run const&, edm::EventSetup const&) override;
    edm::ESWatcher<HGCalCondSerializablePedestalsRcd> cfgWatcher_;
    edm::ESGetToken<HGCalCondSerializablePedestals, HGCalCondSerializablePedestalsRcd> tokenConds_;
    edm::ESGetToken<HGCalCondSerializableModuleInfo, HGCalCondSerializableModuleInfoRcd> moduleInfoToken_;

    const edm::EDGetTokenT<hgcaldigi::HGCalDigiHostCollection> digisToken_;
    const device::EDPutToken<hgcalrechit::HGCalRecHitDeviceCollection> recHitsToken_;
    HGCalRecHitCalibrationAlgorithms calibrator_;  // cannot be "const" because the calibrate() method is not const
    // HGCalCalibrationParameterProvider calibrationParameterProvider_;
    int n_hits_scale;
  };

  HGCalRecHitProducer::HGCalRecHitProducer(const edm::ParameterSet& iConfig)
      : tokenConds_(esConsumes<HGCalCondSerializablePedestals, HGCalCondSerializablePedestalsRcd>(
          edm::ESInputTag(iConfig.getParameter<std::string>("pedestal_label")))),
        moduleInfoToken_(esConsumes<HGCalCondSerializableModuleInfo,HGCalCondSerializableModuleInfoRcd,edm::Transition::BeginRun>(
          iConfig.getParameter<edm::ESInputTag>("ModuleInfo"))),
        digisToken_{consumes<hgcaldigi::HGCalDigiHostCollection>(iConfig.getParameter<edm::InputTag>("digis"))},
        recHitsToken_{produces()},
        calibrator_{HGCalRecHitCalibrationAlgorithms(
          iConfig.getParameter<int>("n_blocks"),
          iConfig.getParameter<int>("n_threads"))},
        // calibrationParameterProvider_(),
        n_hits_scale{iConfig.getParameter<int>("n_hits_scale")}
    {}
    
  void HGCalRecHitProducer::beginRun(edm::Run const& iRun, edm::EventSetup const& iSetup){
    // auto moduleInfo = iSetup.getData(moduleInfoToken_);
    // std::tuple<uint16_t,uint8_t,uint8_t,uint8_t> denseIdxMax = moduleInfo.getMaxValuesForDenseIndex();    
    // calibrationParameterProvider_.initialize(HGCalCalibrationParameterProviderConfig{.EventSLinkMax=std::get<0>(denseIdxMax),
    //                                   .sLinkCaptureBlockMax=std::get<1>(denseIdxMax),
    //                                   .captureBlockECONDMax=std::get<2>(denseIdxMax),
    //                                   .econdERXMax=std::get<3>(denseIdxMax),
    //                                   .erxChannelMax = 37+2,//+2 for the two common modes
    // });
  }

  void HGCalRecHitProducer::produce(device::Event& iEvent, device::EventSetup const& iSetup) {
    auto queue = iEvent.queue();
    
    // Read digis
    auto const& hostDigisIn = iEvent.get(digisToken_);
    int oldSize = hostDigisIn.view().metadata().size();
    int newSize = oldSize * n_hits_scale;
    auto hostDigis = HGCalDigiHostCollection(newSize, queue);

    // // Check if there are new conditions and read them
    // if (cfgWatcher_.check(iSetup)){

    //   auto conds = iSetup.getData(tokenConds_);
    //   size_t nconds = conds.params_.size();
    //   LogDebug("HGCalCalibrationParamter") << "Conditions retrieved:\n" << nconds;

    //   // Print out all conditions readout
    //   HGCalRecHitCalibrationAlgorithms::CalibParams calibParams;
    //   LogDebug("HGCalCalibrationParamter") << "   ID  eRx  Channel  isCM?  Pedestal  CM slope  CM offset  kappa(BX-1)" << std::endl;
    //   for(auto it : conds.params_) {
    //     HGCalElectronicsId id(it.first);
    //     HGCalFloatPedestals table = conds.getFloatPedestals(it.second);
    //     // calibrationParameterProvider_[id.raw()].pedestal = table.pedestal;
    //     // calibrationParameterProvider_[id.raw()].cm_slope = table.cm_slope;
    //     // calibrationParameterProvider_[id.raw()].cm_offset = table.cm_offset;
    //     // calibrationParameterProvider_[id.raw()].kappa_bxm1 = table.kappa_bxm1;

    //     // LogDebug("HGCalCalibrationParamter") << std::setw(5) << std::dec << (uint32_t)id.raw() << " " << std::setw(4) << std::dec << (uint32_t)id.econdeRx() << " "
    //     //           << std::setw(8) << (uint32_t)id.halfrocChannel() << " " << std::setw(6) << (uint32_t)id.isCM() << " "
    //     //           << std::setw(9) << std::setprecision(3) << calibrationParameterProvider_[id.raw()].pedestal << " " << std::setw(9) << calibrationParameterProvider_[id.raw()].cm_slope << " "
    //     //           << std::setw(10) << calibrationParameterProvider_[id.raw()].cm_offset << " " << std::setw(12) << calibrationParameterProvider_[id.raw()].kappa_bxm1;
    //   }

    //   
    //   }

    for(int i=0; i<newSize;i++){
      hostDigis.view()[i].electronicsId() = hostDigisIn.view()[i%oldSize].electronicsId();
      hostDigis.view()[i].tctp() = hostDigisIn.view()[i%oldSize].tctp();
      hostDigis.view()[i].adcm1() = hostDigisIn.view()[i%oldSize].adcm1();
      hostDigis.view()[i].adc() = hostDigisIn.view()[i%oldSize].adc();
      hostDigis.view()[i].tot() = hostDigisIn.view()[i%oldSize].tot();
      hostDigis.view()[i].toa() = hostDigisIn.view()[i%oldSize].toa();
      hostDigis.view()[i].cm() = hostDigisIn.view()[i%oldSize].cm();
      hostDigis.view()[i].flags() = hostDigisIn.view()[i%oldSize].flags();
    }

    LogDebug("HGCalRecHitProducer") << "Loaded host digis: " << hostDigis.view().metadata().size() << std::endl;

    LogDebug("HGCalRecHitProducer") << "\n\nINFO -- calling calibrate method" << std::endl;

    auto start = now();
    auto recHits = calibrator_.calibrate(queue, hostDigis);
    alpaka::wait(queue);
    auto stop = now();

    LogDebug("HGCalRecHitProducer") << "Time: " << duration(start, stop) << std::endl;

    LogDebug("HGCalRecHitProducer") << "\n\nINFO -- storing rec hits in the event" << std::endl;
    iEvent.emplace(recHitsToken_, std::move(*recHits));
  }

  void HGCalRecHitProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("digis", edm::InputTag("hgcalDigis", "DIGI", "TEST"));
    desc.add<int>("n_blocks", -1);
    desc.add<int>("n_threads", -1);
    desc.add<int>("n_hits_scale", -1);
    desc.add<std::string>("pedestal_label", "");
    desc.add<edm::ESInputTag>("ModuleInfo",edm::ESInputTag(""));
    descriptions.addWithDefaultLabel(desc);
  }

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

// define this as a plug-in
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/MakerMacros.h"
DEFINE_FWK_ALPAKA_MODULE(HGCalRecHitProducer);
