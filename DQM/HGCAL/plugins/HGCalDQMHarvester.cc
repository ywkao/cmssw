#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/FileInPath.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/Transition.h"

#include "DQMServices/Core/interface/DQMEDHarvester.h"
#include "DQMServices/Core/interface/DQMStore.h"

#include "CondFormats/DataRecord/interface/HGCalElectronicsMappingRcd.h"
#include "CondFormats/DataRecord/interface/HGCalModuleConfigurationRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalConfiguration.h"
#include "CondFormats/HGCalObjects/interface/HGCalMappingModuleIndexer.h"
#include "CondFormats/HGCalObjects/interface/HGCalMappingModuleIndexerTrigger.h"
#include "CondFormats/HGCalObjects/interface/HGCalMappingParameterHost.h"

#include "DQM/HGCAL/interface/HGCalDQMGeometry.h"
#include "DQM/HGCAL/interface/HGCalDQMWorkerBase.h"
#include "DQM/HGCAL/interface/HGCalSysValDQMCommon.h"

#include "DQM/HGCAL/interface/HGCalChannelWorker.h"
#include "DQM/HGCAL/interface/HGCalFedWorker.h"
#include "DQM/HGCAL/interface/HGCalLSWorker.h"
#include "DQM/HGCAL/interface/HGCalQualityWorker.h"
#include "DQM/HGCAL/interface/HGCalTriggerWorker.h"

/**
 * \class HGCalDQMHarvester
 *
 * DQMEDHarvester that post-processes the HGCal DQM client histograms. At the
 * first end of lumisection it builds the HGCalDQMGeometry from the electronics
 * mapping and module configuration, then runs its workers each lumisection
 * and at end of run. The workers are HGCalQualityWorker, HGCalChannelWorker
 * (unless SkipSlowStream), HGCalTriggerWorker (unless SkipTriggerDQM),
 * HGCalLSWorker and HGCalFedWorker. Thresholds come from the
 * dqmQualityThreshold JSON.
 */
class HGCalDQMHarvester : public DQMEDHarvester {
public:
  explicit HGCalDQMHarvester(edm::ParameterSet const& ps);
  ~HGCalDQMHarvester() override = default;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

protected:
  void dqmEndLuminosityBlock(DQMStore::IBooker&,
                             DQMStore::IGetter&,
                             edm::LuminosityBlock const&,
                             edm::EventSetup const&) override;
  void dqmEndRun(DQMStore::IBooker&, DQMStore::IGetter&, edm::Run const&, edm::EventSetup const&) override;
  void dqmEndJob(DQMStore::IBooker&, DQMStore::IGetter&) override {}

private:
  edm::ESGetToken<HGCalMappingModuleIndexer, HGCalElectronicsMappingRcd> moduleIdxTkn_;
  edm::ESGetToken<HGCalMappingModuleIndexerTrigger, HGCalElectronicsMappingRcd> moduleIdxTriggerTkn_;
  edm::ESGetToken<hgcal::HGCalMappingModuleParamHost, HGCalElectronicsMappingRcd> moduleInfoTkn_;
  edm::ESGetToken<hgcal::HGCalMappingModuleTriggerParamHost, HGCalElectronicsMappingRcd> moduleInfoTriggerTkn_;
  edm::ESGetToken<HGCalConfiguration, HGCalModuleConfigurationRcd> moduleConfigTkn_;

  std::unique_ptr<hgcal::dqm::HGCalDQMGeometry> geometry_;
  hgcal::dqm::ErrorSummarizer error_summarizer_;
  hgcal::dqm::EcontErrorSummarizer econt_error_summarizer_;
  std::vector<std::unique_ptr<hgcal::dqm::HGCalDQMWorkerBase>> workers_;
  bool firstLS_;
};

namespace {
  nlohmann::json loadJson(std::string const& fipPath) {
    edm::FileInPath fip(fipPath);
    std::ifstream f(fip.fullPath());
    return nlohmann::json::parse(f);
  }
}  // namespace

HGCalDQMHarvester::HGCalDQMHarvester(edm::ParameterSet const& ps)
    : moduleIdxTkn_(esConsumes<edm::Transition::EndLuminosityBlock>()),
      moduleIdxTriggerTkn_(esConsumes<edm::Transition::EndLuminosityBlock>()),
      moduleInfoTkn_(esConsumes<edm::Transition::EndLuminosityBlock>()),
      moduleInfoTriggerTkn_(esConsumes<edm::Transition::EndLuminosityBlock>()),
      moduleConfigTkn_(esConsumes<edm::Transition::EndLuminosityBlock>()),
      geometry_(std::make_unique<hgcal::dqm::HGCalDQMGeometry>(ps.getParameter<std::string>("TemplateFiles"),
                                                               std::string("/geometry_v16.5.root"),
                                                               ps.getParameter<bool>("SkipTriggerDQM"),
                                                               ps.getParameter<std::string>("Era"))),
      error_summarizer_(
          loadJson(ps.getParameter<std::string>("dqmQualityThreshold")).value("econd", nlohmann::json({}))),
      econt_error_summarizer_(
          loadJson(ps.getParameter<std::string>("dqmQualityThreshold")).value("econt", nlohmann::json({}))),
      firstLS_(true) {
  auto folderRoot = ps.getParameter<std::string>("FolderRoot");
  bool skipTriggerDQM = ps.getParameter<bool>("SkipTriggerDQM");
  bool skipSlowStream = ps.getParameter<bool>("SkipSlowStream");
  bool enableOverflowM = ps.getParameter<bool>("EnableOverflowMarkers");

  float overflowThreshold = error_summarizer_.getThreshold("stdadc_overflow_threshold");
  float saturatedAdcThreshold = error_summarizer_.getThreshold("saturated_adc_threshold");

  workers_.push_back(std::make_unique<hgcal::dqm::HGCalQualityWorker>(folderRoot, error_summarizer_));

  if (!skipSlowStream) {
    workers_.push_back(std::make_unique<hgcal::dqm::HGCalChannelWorker>(
        folderRoot, error_summarizer_, overflowThreshold, saturatedAdcThreshold, enableOverflowM));
  }

  if (!skipTriggerDQM) {
    workers_.push_back(std::make_unique<hgcal::dqm::HGCalTriggerWorker>(folderRoot, econt_error_summarizer_));
  }

  workers_.push_back(std::make_unique<hgcal::dqm::HGCalLSWorker>(
      folderRoot, error_summarizer_, econt_error_summarizer_, skipTriggerDQM));

  workers_.push_back(std::make_unique<hgcal::dqm::HGCalFedWorker>(folderRoot));
}

void HGCalDQMHarvester::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<std::string>("TemplateFiles", "HGCalCommissioning/DQM/data");
  desc.add<std::string>("Era", "");
  desc.add<bool>("SkipTriggerDQM", false);
  desc.add<bool>("SkipSlowStream", false);
  desc.add<bool>("EnableOverflowMarkers", true);
  desc.add<std::string>("FolderRoot", "HGCAL_v2");
  desc.add<std::string>("dqmQualityThreshold", "HGCalCommissioning/DQM/data/dqm_quality_threshold.json");
  descriptions.addWithDefaultLabel(desc);
}

void HGCalDQMHarvester::dqmEndLuminosityBlock(DQMStore::IBooker& ibooker,
                                              DQMStore::IGetter& igetter,
                                              edm::LuminosityBlock const& iLumi,
                                              edm::EventSetup const& iSetup) {
  if (firstLS_) {
    geometry_->build(
        iSetup, moduleIdxTkn_, moduleIdxTriggerTkn_, moduleInfoTkn_, moduleInfoTriggerTkn_, moduleConfigTkn_);
    for (auto& w : workers_)
      w->book(ibooker, *geometry_);
    firstLS_ = false;
  }
  for (auto& w : workers_)
    w->endLumi(ibooker, igetter, *geometry_, iLumi);
}

void HGCalDQMHarvester::dqmEndRun(DQMStore::IBooker& ibooker,
                                  DQMStore::IGetter& igetter,
                                  edm::Run const& /*run*/,
                                  edm::EventSetup const& /*iSetup*/) {
  for (auto& w : workers_)
    w->endRun(ibooker, igetter, *geometry_);
}

DEFINE_FWK_MODULE(HGCalDQMHarvester);
