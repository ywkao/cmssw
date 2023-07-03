#include "DataFormats/PortableTestObjects/interface/alpaka/TestDeviceCollection.h"
#include "DataFormats/HGCalDigi/interface/HGCalDigiHostCollection.h"
#include "DataFormats/HGCalDigi/interface/alpaka/HGCalDigiDeviceCollection.h"
#include "DataFormats/HGCalRecHit/interface/HGCalRecHitHostCollection.h"
#include "DataFormats/HGCalRecHit/interface/alpaka/HGCalRecHitDeviceCollection.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDPutToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/Event.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EventSetup.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/global/EDProducer.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"

#include "HGCalRecHitCalibrationAlgorithms.h"

// includes for loading pedestal txt file
#include <iomanip> // for std::setw
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESWatcher.h"
#include "CondFormats/DataRecord/interface/HGCalCondSerializablePedestalsRcd.h"
#include "CondFormats/DataRecord/interface/HGCalCondSerializableConfigRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializablePedestals.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializableConfig.h"

// includes for size parameters
#include "CondFormats/DataRecord/interface/HGCalCondSerializableModuleInfoRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializableModuleInfo.h"

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

  class HGCalRecHitProducer : public global::EDProducer<> {
  public:
    HGCalRecHitProducer(edm::ParameterSet const& config)
        : deviceToken_{produces()}, size_{config.getParameter<int32_t>("size")},
          //digisToken_{consumes<hgcaldigi::HGCalDigiHostCollection>(config.getParameter<edm::InputTag>("digis"))},
          //recHitsToken_{produces()},
          algo_{HGCalRecHitCalibrationAlgorithms(
            config.getParameter<int>("n_blocks"),
            config.getParameter<int>("n_threads"))}
    {}

    void produce(edm::StreamID sid, device::Event& event, device::EventSetup const&) const override {
      // run the algorithm, potentially asynchronously
      portabletest::TestDeviceCollection deviceProduct{size_, event.queue()};
      algo_.fill(event.queue(), deviceProduct);

      // put the asynchronous product into the event without waiting
      event.emplace(deviceToken_, std::move(deviceProduct));

      /*
      auto queue = event.queue();
      auto const& hostDigis = event.get(digisToken_);
      
      // // Read digis
      // auto const& hostDigisIn = event.get(digisToken_);
      // int oldSize = hostDigisIn.view().metadata().size();
      // int newSize = oldSize * n_hits_scale;
      // auto hostDigis = HGCalDigiHostCollection(newSize, queue);

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
      //     calibrationParameterProvider_[id.raw()].pedestal = table.pedestal;
      //     calibrationParameterProvider_[id.raw()].cm_slope = table.cm_slope;
      //     calibrationParameterProvider_[id.raw()].cm_offset = table.cm_offset;
      //     calibrationParameterProvider_[id.raw()].kappa_bxm1 = table.kappa_bxm1;

      //     LogDebug("HGCalCalibrationParamter") << std::setw(5) << std::dec << (uint32_t)id.raw() << " " << std::setw(4) << std::dec << (uint32_t)id.econdeRx() << " "
      //               << std::setw(8) << (uint32_t)id.halfrocChannel() << " " << std::setw(6) << (uint32_t)id.isCM() << " "
      //               << std::setw(9) << std::setprecision(3) << calibrationParameterProvider_[id.raw()].pedestal << " " << std::setw(9) << calibrationParameterProvider_[id.raw()].cm_slope << " "
      //               << std::setw(10) << calibrationParameterProvider_[id.raw()].cm_offset << " " << std::setw(12) << calibrationParameterProvider_[id.raw()].kappa_bxm1;
      //   }
      // }

      // for(int i=0; i<newSize;i++){
      //   hostDigis.view()[i].electronicsId() = hostDigisIn.view()[i%oldSize].electronicsId();
      //   hostDigis.view()[i].tctp() = hostDigisIn.view()[i%oldSize].tctp();
      //   hostDigis.view()[i].adcm1() = hostDigisIn.view()[i%oldSize].adcm1();
      //   hostDigis.view()[i].adc() = hostDigisIn.view()[i%oldSize].adc();
      //   hostDigis.view()[i].tot() = hostDigisIn.view()[i%oldSize].tot();
      //   hostDigis.view()[i].toa() = hostDigisIn.view()[i%oldSize].toa();
      //   hostDigis.view()[i].cm() = hostDigisIn.view()[i%oldSize].cm();
      //   hostDigis.view()[i].flags() = hostDigisIn.view()[i%oldSize].flags();
      // }

      std::cout << "Loaded host digis: " << hostDigis.view().metadata().size() << std::endl;

      std::cout << "\n\nINFO -- calling calibrate method" << std::endl;

      auto start = now();
      auto recHits = algo_.calibrate(queue, hostDigis);
      alpaka::wait(queue);
      auto stop = now();

      std::cout<<"Time: "<< duration(start, stop) <<std::endl;

      std::cout << "\n\nINFO -- storing rec hits in the event" << std::endl;
      event.emplace(recHitsToken_, std::move(*recHits));
      */
    }

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
      edm::ParameterSetDescription desc;
      desc.add<int32_t>("size");
      desc.add<int>("n_blocks");
      desc.add<int>("n_threads");
      descriptions.addWithDefaultLabel(desc);
    }

  private:
    const device::EDPutToken<portabletest::TestDeviceCollection> deviceToken_;
    const int32_t size_;
    //const edm::EDGetTokenT<hgcaldigi::HGCalDigiHostCollection> digisToken_;
    //const edm::EDPutTokenT<hgcalrechit::HGCalRecHitDeviceCollection> recHitsToken_;

    // implementation of the algorithm
    HGCalRecHitCalibrationAlgorithms algo_;
  };

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#include "HeterogeneousCore/AlpakaCore/interface/alpaka/MakerMacros.h"
DEFINE_FWK_ALPAKA_MODULE(HGCalRecHitProducer);
