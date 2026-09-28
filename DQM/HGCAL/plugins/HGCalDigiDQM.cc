#include <algorithm>
#include <cassert>
#include <limits>
#include <string>
#include <map>
#include <utility>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "DQMServices/Core/interface/DQMEDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DQMServices/Core/interface/MonitorElement.h"
#include "DataFormats/HGCalDigi/interface/HGCalDigiHost.h"
#include "DataFormats/HGCalDigi/interface/HGCalRawDataDefinitions.h"
#include "HGCalCommissioning/SystemTestEventFilters/interface/HGCalTestSystemMetaData.h"
#include "CondFormats/DataRecord/interface/HGCalDenseIndexInfoRcd.h"
#include "CondFormats/DataRecord/interface/HGCalElectronicsMappingRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalMappingModuleIndexer.h"
#include "CondFormats/HGCalObjects/interface/HGCalMappingParameterHost.h"
#include "DQM/HGCAL/interface/HGCalSysValDQMCommon.h"

/**
 * \class HGCalDigiDQM
 *
 * DQM client for HGCal digis. Reads digis in SoA format with the trigger-time
 * metadata and fills per-module, per-channel profiles and distributions of ADC,
 * ADC(-1), ADC-ADC(-1), CM, TOT and TOA. Once 500 events have been seen it picks,
 * once per job, a seed channel per module (highest mean TOT, else highest mean
 * ADC-ADC(-1)) and fills its ADC/TOT/TOA vs trigger phase. It processes the first
 * MinimumEvents events and then every PrescaleFactor-th event. HGCalDQMHarvester
 * (HGCalChannelWorker) turns these into summary plots.
 */
class HGCalDigiDQM : public DQMEDAnalyzer {
typedef std::pair<uint32_t,uint32_t> MonitoredElementKey_t;
struct MonitoredElement_t {
  std::string typecode;
  bool zside, isSiPM;
  uint32_t layer, i1, i2, nErx, dqmIndex;
  uint32_t fedid, modid, econdidx, cassette, endcap, moduleIndex;
};
public:
  explicit HGCalDigiDQM(const edm::ParameterSet&);
  ~HGCalDigiDQM() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void bookHistograms(DQMStore::IBooker&, edm::Run const&, edm::EventSetup const&) override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void bookModuleHistograms(DQMStore::IBooker&, const MonitoredElementKey_t&);
  void findModuleSeeds(uint32_t minProcessed=500);

  // ------------ member data ------------
  edm::EDGetTokenT<hgcaldigi::HGCalDigiHost> digisTkn_;
  const edm::EDGetTokenT<HGCalTestSystemTrigTimeCollection> metaDataTkn_;
  edm::ESGetToken<hgcal::HGCalDenseIndexInfoHost, HGCalDenseIndexInfoRcd> denseIndexInfoTkn_;

  edm::ESGetToken<HGCalMappingModuleIndexer, HGCalElectronicsMappingRcd> moduleIdxTkn_;
  edm::ESGetToken<hgcal::HGCalMappingModuleParamHost, HGCalElectronicsMappingRcd> moduleInfoTkn_;
  const unsigned int minEvents_;
  const unsigned int prescaleFactor_;
  unsigned int nProcessed_;
  std::map<std::string, std::map<MonitoredElementKey_t, MonitorElement*> > moduleHistos_;
  std::map<MonitoredElementKey_t, MonitoredElement_t> followedModules_;
  std::map<MonitoredElementKey_t, uint32_t> moduleSeeds_;
  int trigTime, trigBx;
};

HGCalDigiDQM::HGCalDigiDQM(const edm::ParameterSet& iConfig)
    : digisTkn_(consumes<hgcaldigi::HGCalDigiHost>(iConfig.getParameter<edm::InputTag>("Digis"))),
      metaDataTkn_(consumes<HGCalTestSystemTrigTimeCollection>(iConfig.getParameter<edm::InputTag>("MetaData"))),
      denseIndexInfoTkn_(esConsumes()),
      moduleIdxTkn_(esConsumes<edm::Transition::BeginRun>()),
      moduleInfoTkn_(esConsumes<edm::Transition::BeginRun>()),
      minEvents_(iConfig.getParameter<unsigned int>("MinimumEvents")),
      prescaleFactor_(std::max(1u, iConfig.getParameter<unsigned int>("PrescaleFactor"))),
      nProcessed_(0) {}

HGCalDigiDQM::~HGCalDigiDQM() {}


void HGCalDigiDQM::bookModuleHistograms(DQMStore::IBooker& ibook, const MonitoredElementKey_t& key) {
 // uint32_t fedId = key.first;
 // uint32_t seq = key.second;

  auto& ele = followedModules_[key];

  std::string endcap = (ele.endcap == 1) ? "Plus" : "Minus";

  // Folder path must match HGCalSysValDigisHarvester's expected layout
  // (cassetteFolder + "(u<i1>-v<i2>) " + typecode), otherwise the harvester
  // looks up nullptrs and crashes in runSlowStream.
  std::string folder = "HGCAL/EndCap_" + endcap +
                     "/Layer_" + std::to_string(ele.layer) +
                     "/Cassette_" + std::to_string(ele.cassette) +
                     "/(u" + std::to_string(ele.i1) +
                     "-v" + std::to_string(ele.i2) + ") " +
                     ele.typecode;

  ibook.setCurrentFolder(folder);

  //Per module histograms
  size_t nch = ele.nErx*37;

  std::string typecode = ele.typecode;

  moduleHistos_["avgadc"][key] = ibook.bookProfile("avgadc", typecode + ";Channel; <ADC>", nch, -0.5, nch-0.5, 100, 0, 1024, "s");
  moduleHistos_["avgcm2"][key] = ibook.bookProfile("avgcm2", typecode + ";Channel; <CM_{2}>", nch, -0.5, nch-0.5, 100, 0, 1024, "s");
  moduleHistos_["adc"][key] = ibook.book1D("adc", typecode + ";ADC; Counts (all channels)", 100, 0, 1024);
  moduleHistos_["avgtot"][key] = ibook.bookProfile("avgtot", typecode + ";Channel; <TOT>", nch, -0.5, nch-0.5, 100, 0, 1024, "s");
  moduleHistos_["tot"][key] = ibook.book1D("tot", typecode + ";TOT; Counts (all channels)", 100, 0, 4096);
  moduleHistos_["avgadcm1"][key] = ibook.bookProfile("avgadcm1", typecode + ";Channel; <ADC(-1)>", nch, -0.5, nch-0.5, 100, 0, 1024, "s");
  moduleHistos_["adcm1"][key] = ibook.book1D("adcm1", typecode + ";ADC_{-1}; Counts (all channels)", 100, 0, 1024);
  moduleHistos_["avgtoa"][key] = ibook.bookProfile("avgtoa", typecode + ";Channel; <TOA>", nch, -0.5, nch-0.5, 100, 0, 1024, "s");
  moduleHistos_["toa"][key] = ibook.book1D("toa", typecode + ";TOA; Counts (all channels)", 100, 0, 1024);
  moduleHistos_["avgdeltaadc"][key] = ibook.bookProfile("avgdeltaadc", typecode + ";Channel; <ADC-ADC_{-1}>", nch, -0.5, nch-0.5, 100, -1024, 1024, "s");
  moduleHistos_["deltaadc"][key] = ibook.book1D("deltaadc", typecode + ";ADC-ADC_{-1}; Counts (all channels)", 150, -49.5, 200.5);
  moduleHistos_["seedadc"][key] = ibook.book1D("seedadc", typecode + ";ADC of channel with max <ADC-ADC_{-1}>; Counts", 100, 0, 1024);
  moduleHistos_["seedadcvstrigtime"][key] = ibook.book2D(
      "seedadcvstrigtime", typecode + ";trigger phase; ADC of channel with max <ADC-ADC_{-1}> or TOT", 224, -111.5, 112.5, 100, 0, 1024);
  moduleHistos_["seedtot"][key] = ibook.book1D("seedtot", typecode + ";TOT of channel with max <ADC-ADC_{-1}> or TOT; Counts", 100, 0, 4096);
  moduleHistos_["seedtotvstrigtime"][key] = ibook.book2D(
      "seedtotvstrigtime", typecode + ";trigger phase; TOT of channel with max <ADC-ADC_{-1}> or TOT", 224, -111.5, 112.5, 100, 0, 4096);
  moduleHistos_["seedtoa"][key] = ibook.book1D("seedtoa", typecode + ";TOA of channel with max <ADC-ADC_{-1}>; Counts", 100, 0, 1024);
  moduleHistos_["seedtoavstrigtime"][key] = ibook.book2D(
      "seedtoavstrigtime", typecode + ";trigger phase; TOA of channel with max <ADC-ADC_{-1}>", 224, -111.5, 112.5, 100, 0, 1024);
}

void HGCalDigiDQM::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  ++nProcessed_;

  trigTime = 0;
  trigBx = 0;
  const auto& metadataHandle = iEvent.getHandle(metaDataTkn_);
  if (metadataHandle.isValid() && metadataHandle->size() > 0) {
    for (size_t index = 0; index < metadataHandle->size(); ++index) {
      const auto& link = metadataHandle->at(index);
      if (!link.valid_)
        continue;
      trigTime = link.time_;
      trigBx = link.bx_;
      break;
    }
  }

  bool toProcess = (nProcessed_ < minEvents_) || (nProcessed_ % prescaleFactor_ == 0);
  if (!toProcess)
    return;

  const auto& digis = iEvent.getHandle(digisTkn_);
  if (!digis.isValid())
    return;

  const auto& digis_view = digis->const_view();
  int32_t ndigis = digis_view.metadata().size();

  const auto& denseIndexInfo = iSetup.getData(denseIndexInfoTkn_);
  const auto& denseIndexInfo_view = denseIndexInfo.const_view();
  int32_t ndii = denseIndexInfo_view.metadata().size();

  assert(ndigis == ndii);

  // Cache references to per-type inner maps once per event. Using at() throws
  // if a histogram type was never booked, which is a real bug we want surfaced.
  // This avoids the non-const operator[] on the outer map, which would insert
  // an empty inner map on a missing key.
  auto& mh_avgadc      = moduleHistos_.at("avgadc");
  auto& mh_avgcm2      = moduleHistos_.at("avgcm2");
  auto& mh_adc         = moduleHistos_.at("adc");
  auto& mh_avgadcm1    = moduleHistos_.at("avgadcm1");
  auto& mh_adcm1       = moduleHistos_.at("adcm1");
  auto& mh_avgdeltaadc = moduleHistos_.at("avgdeltaadc");
  auto& mh_deltaadc    = moduleHistos_.at("deltaadc");
  auto& mh_avgtot      = moduleHistos_.at("avgtot");
  auto& mh_tot         = moduleHistos_.at("tot");
  auto& mh_avgtoa      = moduleHistos_.at("avgtoa");
  auto& mh_toa         = moduleHistos_.at("toa");
  auto& mh_seedadc     = moduleHistos_.at("seedadc");
  auto& mh_seedadcvstt = moduleHistos_.at("seedadcvstrigtime");
  auto& mh_seedtot     = moduleHistos_.at("seedtot");
  auto& mh_seedtotvstt = moduleHistos_.at("seedtotvstrigtime");
  auto& mh_seedtoa     = moduleHistos_.at("seedtoa");
  auto& mh_seedtoavstt = moduleHistos_.at("seedtoavstrigtime");

  // Cache per-key iterators so we only refresh when the key changes (digis are
  // grouped by module, so this saves lookups). Using find() also prevents
  // operator[] from inserting nullptr entries for keys that were not booked.
  MonitoredElementKey_t cur_key(std::numeric_limits<uint32_t>::max(),
                                std::numeric_limits<uint32_t>::max());
  bool cur_key_ok = false;
  auto it_avgadc      = mh_avgadc.end();
  auto it_avgcm2      = mh_avgcm2.end();
  auto it_adc         = mh_adc.end();
  auto it_avgadcm1    = mh_avgadcm1.end();
  auto it_adcm1       = mh_adcm1.end();
  auto it_avgdeltaadc = mh_avgdeltaadc.end();
  auto it_deltaadc    = mh_deltaadc.end();
  auto it_avgtot      = mh_avgtot.end();
  auto it_tot         = mh_tot.end();
  auto it_avgtoa      = mh_avgtoa.end();
  auto it_toa         = mh_toa.end();
  auto it_seed        = moduleSeeds_.end();

  std::map<MonitoredElementKey_t, uint32_t> seedDigiIdx;
  for (int32_t i = 0; i < ndigis; ++i) {
    auto indexinfo = denseIndexInfo_view[i];
    MonitoredElementKey_t key(indexinfo.fedId(), indexinfo.fedReadoutSeq());

    if (key != cur_key) {
      cur_key = key;
      cur_key_ok = false;
      if (followedModules_.find(key) == followedModules_.end())
        continue;
      it_avgadc      = mh_avgadc.find(key);
      it_avgcm2      = mh_avgcm2.find(key);
      it_adc         = mh_adc.find(key);
      it_avgadcm1    = mh_avgadcm1.find(key);
      it_adcm1       = mh_adcm1.find(key);
      it_avgdeltaadc = mh_avgdeltaadc.find(key);
      it_deltaadc    = mh_deltaadc.find(key);
      it_avgtot      = mh_avgtot.find(key);
      it_tot         = mh_tot.find(key);
      it_avgtoa      = mh_avgtoa.find(key);
      it_toa         = mh_toa.find(key);
      it_seed        = moduleSeeds_.find(key);
      cur_key_ok =
          it_avgadc != mh_avgadc.end() && it_avgcm2 != mh_avgcm2.end() &&
          it_adc != mh_adc.end() && it_avgadcm1 != mh_avgadcm1.end() &&
          it_adcm1 != mh_adcm1.end() && it_avgdeltaadc != mh_avgdeltaadc.end() &&
          it_deltaadc != mh_deltaadc.end() && it_avgtot != mh_avgtot.end() &&
          it_tot != mh_tot.end() && it_avgtoa != mh_avgtoa.end() &&
          it_toa != mh_toa.end();
    }
    if (!cur_key_ok) continue;

    uint32_t chIdx = indexinfo.chNumber();

    if (it_seed != moduleSeeds_.end() && it_seed->second == chIdx)
      seedDigiIdx[key] = i;

    auto digi = digis_view[i];
    uint8_t tctp = digi.tctp();
    double adc = digi.adc();
    double tot = digi.tot();
    double toa = digi.toa();
    double adcm = digi.adcm1();
    double cmsum = digi.cm();
    uint16_t flags = digi.flags();
    double deltaadc = adc - adcm;

    if(flags==hgcal::DIGI_FLAG::NotAvailable) continue;
    if(adc<0 && tot<0) continue;

    // ADC mode
    if ( tctp==0 ) {
      it_avgadc->second->Fill(chIdx,adc);
      it_avgcm2->second->Fill(chIdx,0.5*cmsum);
      it_adc->second->Fill(adc);
      it_avgadcm1->second->Fill(chIdx,adcm);
      it_adcm1->second->Fill(adcm);
      it_avgdeltaadc->second->Fill(chIdx,deltaadc);
      it_deltaadc->second->Fill(deltaadc);
    }

    // TOT mode
    if ( tctp==3 ) {
      it_avgtot->second->Fill(chIdx,tot);
      it_tot->second->Fill(tot);
    }

    //Valid TOA
    if ( toa>0 ) {
      it_avgtoa->second->Fill(chIdx,toa);
      it_toa->second->Fill(toa);
    }
  }

  findModuleSeeds();

  //histograms for best S/N candidate cells
  if(seedDigiIdx.empty()) return;
  for(auto it : seedDigiIdx) {
    auto key = it.first;
    auto digiIdx = it.second;
    auto digi = digis_view[digiIdx];
    auto adc = digi.adc();
    auto adcm = digi.adcm1();
    auto tot = digi.tot();
    auto toa = digi.toa();
    auto tctp = digi.tctp();

    auto it_seedadc_k     = mh_seedadc.find(key);
    auto it_seedadcvstt_k = mh_seedadcvstt.find(key);
    if (it_seedadc_k == mh_seedadc.end() || it_seedadcvstt_k == mh_seedadcvstt.end())
      continue;
    if (tctp == 0) {
      double deltaadc = adc - adcm;
      it_seedadc_k->second->Fill(deltaadc);
      it_seedadcvstt_k->second->Fill(trigTime, deltaadc);
    } else if (tctp == 3) {
      auto it_seedtot_k     = mh_seedtot.find(key);
      auto it_seedtotvstt_k = mh_seedtotvstt.find(key);
      if (it_seedtot_k != mh_seedtot.end() && it_seedtotvstt_k != mh_seedtotvstt.end()) {
        it_seedtot_k->second->Fill(tot);
        it_seedtotvstt_k->second->Fill(trigTime, tot);
      }
    }
    if (toa > 0) {
      auto it_seedtoa_k     = mh_seedtoa.find(key);
      auto it_seedtoavstt_k = mh_seedtoavstt.find(key);
      if (it_seedtoa_k != mh_seedtoa.end() && it_seedtoavstt_k != mh_seedtoavstt.end()) {
        it_seedtoa_k->second->Fill(digi.toa());
        it_seedtoavstt_k->second->Fill(trigTime, digi.toa());
      }
    }
  }
}

void HGCalDigiDQM::findModuleSeeds(uint32_t minProcessed) {

  if(nProcessed_<minProcessed) return;
  if(moduleSeeds_.size()>0) return;

  auto outer_it = moduleHistos_.find("avgdeltaadc");
  if (outer_it == moduleHistos_.end()) return;

  //loop over sums histos
  for(const auto& it : outer_it->second) {
    auto key = it.first;
    auto h = it.second;
    auto htot = moduleHistos_["avgtot"][key];

    //find the channel which is more promising in S/N
    double maxDeltaADC(-1), maxTOT(-1);
    int seedCandidate(0);
    for(int xbin=0; xbin<h->getNbinsX(); xbin++) {
      double deltaADC = h->getBinContent(xbin+1);
      double deltaADC_unc = h->getBinError(xbin+1);
      double tot = htot->getBinContent(xbin+1);
      double tot_unc = htot->getBinError(xbin+1);
      if(tot>0 && tot>maxTOT && tot_unc>0) {
        maxTOT = tot;
        seedCandidate = xbin;
      }
      else if(deltaADC>0 && deltaADC>maxDeltaADC && deltaADC_unc>0) {
        maxDeltaADC = deltaADC;
        seedCandidate = xbin;
      }
    }

    //set as seed
    moduleSeeds_[key]=seedCandidate;
  }
}
void HGCalDigiDQM::bookHistograms(DQMStore::IBooker& ibook,
                                  edm::Run const& run,
                                  edm::EventSetup const& iSetup) {
  // Module MAP
  const HGCalMappingModuleIndexer& moduleIndexer = iSetup.getData(moduleIdxTkn_);
  const hgcal::HGCalMappingModuleParamHost& moduleInfo = iSetup.getData(moduleInfoTkn_);

  for (const auto& [rawTypecode, fedData] : moduleIndexer.typecodeMap()) {

    const uint32_t fedid = fedData.first;
    const uint32_t imod = fedData.second;

    const uint32_t denseModIdx = moduleIndexer.getIndexForModule(fedid, imod);
    const auto& modInfo = moduleInfo.view()[denseModIdx];

    std::string typecode = rawTypecode;
    std::replace(typecode.begin(), typecode.end(), '-', '_');

    MonitoredElement_t ele;
    ele.dqmIndex = denseModIdx;
    ele.typecode = typecode;
    ele.nErx = moduleIndexer.getNumERxs(fedid, imod);
    ele.zside = modInfo.zside();
    ele.endcap = ele.zside ? 1 : -1;
    ele.isSiPM = modInfo.isSiPM();
    ele.layer = modInfo.plane();
    ele.i1 = modInfo.i1();
    ele.i2 = modInfo.i2();
    ele.fedid = fedid;
    ele.modid = imod;
    ele.econdidx = modInfo.econdidx();
    ele.cassette = modInfo.cassette();

    MonitoredElementKey_t key(fedid, imod);
    followedModules_[key] = ele;

    bookModuleHistograms(ibook, key);
  }
}

void HGCalDigiDQM::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("Digis", edm::InputTag("hgcalDigis", ""));
  desc.add<edm::InputTag>("MetaData", edm::InputTag("hgcalTrigTimeProducer", ""));
  desc.add<unsigned int>("MinimumEvents", 5000);
  desc.add<unsigned int>("PrescaleFactor", 5000);
  descriptions.add("hgcaldigidqm", desc);
}

DEFINE_FWK_MODULE(HGCalDigiDQM);
