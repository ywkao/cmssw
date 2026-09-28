#ifndef HGCalCommissioning_DQM_interface_HGCalFedWorker_h
#define HGCalCommissioning_DQM_interface_HGCalFedWorker_h

#include <string>

#include "HGCalCommissioning/DQM/interface/HGCalDQMWorkerBase.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

namespace hgcal { namespace dqm {

  using MonitorElement = ::dqm::impl::MonitorElement;

  // Projects HGCAL/FED/fedPayload onto its Y axis and books
  // HGCAL/FED/fed_payload_distribution (TH1F). FED-scoped quality MEs
  // (econdQualityFED_*, econtQualityFED_*) are client MEs not touched here.
  class HGCalFedWorker : public HGCalDQMWorkerBase {
  public:
    explicit HGCalFedWorker(std::string folderRoot);
    ~HGCalFedWorker() override = default;

    void book(DQMStore::IBooker&, HGCalDQMGeometry const&) override {}

    void endRun(DQMStore::IBooker&, DQMStore::IGetter&, HGCalDQMGeometry&) override;

  private:
    std::string folderRoot_;
    MonitorElement* me_fed_payload_th1d_ = nullptr;
  };

}}  // hgcal::dqm

#endif
