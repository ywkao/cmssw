// CMSSW includes
#include "DataFormats/HGCalDigi/interface/HGCalDigiHostCollection.h"

#if NOT_WORKING
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

#include "DataFormats/PortableTestObjects/interface/alpaka/TestDeviceCollection.h"

// includes for loading pedestal txt file
#include <iomanip> // for std::setw
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESWatcher.h"
#include "CondFormats/DataRecord/interface/HGCalCondSerializablePedestalsRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializablePedestals.h"
#include "CondFormats/DataRecord/interface/HGCalCondSerializableConfigRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalCondSerializableConfig.h"

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

    //edm::ESWatcher<HGCalCondSerializablePedestalsRcd> cfgWatcher_;
    //edm::ESGetToken<HGCalCondSerializablePedestals, HGCalCondSerializablePedestalsRcd> tokenConds_;
    //const edm::EDGetTokenT<hgcaldigi::HGCalDigiHostCollection> digisToken_;
    //const edm::EDPutTokenT<hgcalrechit::HGCalRecHitHostCollection> recHitsToken_;
    const device::EDPutToken<hgcalrechit::HGCalRecHitDeviceCollection> recHitsToken_;
    //const device::EDPutToken<portabletest::TestDeviceCollection> recHitsToken_;
    //HGCalRecHitCalibrationAlgorithms calibrator_;  // cannot be "const" because the calibrate() method is not const
    //int n_hits_scale;
  };

  HGCalRecHitProducer::HGCalRecHitProducer(const edm::ParameterSet& iConfig)
      : //tokenConds_(esConsumes<HGCalCondSerializablePedestals, HGCalCondSerializablePedestalsRcd>(
        //  edm::ESInputTag(iConfig.getParameter<std::string>("pedestal_label")))),
        //digisToken_{consumes<hgcaldigi::HGCalDigiHostCollection>(iConfig.getParameter<edm::InputTag>("digis"))},
        recHitsToken_{produces()}
        /*
        recHitsToken_{produces()},
        calibrator_{HGCalRecHitCalibrationAlgorithms(
          iConfig.getParameter<int>("n_blocks"),
          iConfig.getParameter<int>("n_threads"))},
        n_hits_scale{iConfig.getParameter<int>("n_hits_scale")}
        */
    {}

  void HGCalRecHitProducer::produce(device::Event& iEvent, device::EventSetup const& iSetup) {
     #if 0
     auto queue = iEvent.queue();

    std::cout << "\n\nINFO -- Start of produce\n\n" << std::endl;
    
    // Read digis
    // auto const& hostDigisIn = iEvent.getHandle(digisToken_);
    // std::cout << "IsValid: " << hostDigisIn.isValid() << std::endl;

    auto const& hostDigisIn = iEvent.get(digisToken_);
    std::cout << "Loaded host digis: " << hostDigisIn.view().metadata().size() << std::endl;
    
    // Check if there are new conditions and read them
    if (cfgWatcher_.check(iSetup)){

      auto conds = iSetup.getData(tokenConds_);
      size_t nconds = conds.params_.size();
      edm::LogInfo("HGCalPedestalsESSourceAnalyzer") << "Conditions retrieved:\n" << nconds;

      // Print out all conditions readout
      HGCalRecHitCalibrationAlgorithms::CalibParams calibParams;
      std::cout << "   ID  eRx  ROC  Channel  isCM?  Pedestal  CM slope  CM offset  kappa(BX-1)" << std::endl;
      for(auto it : conds.params_) {
        HGCalElectronicsId id(it.first);
        bool cmflag = id.isCM();
        uint32_t eRx = (uint32_t) id.econdeRx();
        uint32_t roc = (uint32_t) eRx/2;
        uint32_t ch = id.halfrocChannel();
        HGCalFloatPedestals table = conds.getFloatPedestals(it.second);
        calibParams[id] = table.pedestal;
        std::cout << std::setw(5) << std::hex << id.raw() << " " << std::setw(4) << std::dec << eRx << " "
                  << std::setw(4) << roc << " " << std::setw(8) << ch << " " << std::setw(6) << cmflag << " "
                  << std::setw(9) << std::setprecision(3) << table.pedestal << " " << std::setw(9) << table.cm_slope << " "
                  << std::setw(10) << table.cm_offset << " " << std::setw(12) << table.kappa_bxm1 << std::endl;
      }

      calibrator_.loadCalibParams(calibParams); // TODO: load map: electronicsID -> vector { pedestal }
    }

    // int oldSize = hostDigisIn.view().metadata().size();
    // int newSize = oldSize * n_hits_scale;
    // hgcaldigi::HGCalDigiHostCollection hostDigis(newSize, queue);

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

    // std::cout << "Loaded host digis: " << hostDigis.view().metadata().size() << std::endl;

    std::cout << "\n\nINFO -- calling calibrate method" << std::endl;

    auto start = now();
    // auto recHits = calibrator_.calibrate(queue, hostDigisIn);
    // alpaka::wait(queue);
    auto stop = now();

    std::cout<<"Time: "<< duration(start, stop) <<std::endl;

    // std::cout << "\n\nINFO -- storing rec hits in the event" << std::endl;
    // iEvent.emplace(recHitsToken_, std::move(*recHits));
#endif
    auto recHits = std::make_unique<hgcalrechit::HGCalRecHitDeviceCollection>(10, iEvent.queue());
    //auto recHits = std::make_unique<portabletest::TestDeviceCollection>(10, iEvent.queue());
    iEvent.put(recHitsToken_, std::move(recHits));
  }


 #endif  // NOT_WORKING

#include "DataFormats/PortableTestObjects/interface/alpaka/TestDeviceCollection.h"
#include "DataFormats/TestObjects/interface/ToyProducts.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/stream/EDProducer.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDPutToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ESGetToken.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaTest/interface/AlpakaESTestRecords.h"
#include "HeterogeneousCore/AlpakaTest/interface/AlpakaESTestData.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {
  /**
   * This class demonstrates a stream EDProducer that
   * - consumes a host EDProduct
   * - consumes a device ESProduct
   * - produces a device EDProduct (that gets transferred to host automatically if needed)
   * - optionally uses a product instance label
   */
  class HGCalRecHitProducer : public stream::EDProducer<> {
  public:
    HGCalRecHitProducer(edm::ParameterSet const& config) :
      size_{config.getParameter<int>("n_hits_scale")},
      digisToken_{consumes(config.getParameter<edm::InputTag>("digis"))},
      devicePutToken_{produces()}
    {}

    void produce(device::Event& iEvent, device::EventSetup const& iSetup) override {
      [[maybe_unused]] auto inpData = iEvent.getHandle(digisToken_);

      auto deviceProduct = std::make_unique<portabletest::TestDeviceCollection>(size_, iEvent.queue());
      iEvent.put(devicePutToken_, std::move(deviceProduct));
    }

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  private:
    const int32_t size_;
    edm::EDGetTokenT<hgcaldigi::HGCalDigiHostCollection> digisToken_;
    device::EDPutToken<portabletest::TestDeviceCollection> devicePutToken_;
  };

  void HGCalRecHitProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("digis", edm::InputTag("hgcalDigis", "DIGI", "TEST"));
    desc.add<int>("n_blocks", -1);
    desc.add<int>("n_threads", -1);
    desc.add<int>("n_hits_scale", -1);
    desc.add<std::string>("pedestal_label", "");
    descriptions.addWithDefaultLabel(desc);
  }

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

// define this as a plug-in
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/MakerMacros.h"
DEFINE_FWK_ALPAKA_MODULE(HGCalRecHitProducer);
