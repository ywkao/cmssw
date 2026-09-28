#ifndef HGCalCommissioning_DQM_interface_HGCalQualityWorker_h
#define HGCalCommissioning_DQM_interface_HGCalQualityWorker_h

#include <map>
#include <string>
#include <vector>
#include <unordered_map>

#include "HGCalCommissioning/DQM/interface/HGCalDQMWorkerBase.h"

namespace hgcal { namespace dqm {

  using MonitorElement = ::dqm::impl::MonitorElement;

  class ErrorSummarizer;   // injected; plugin owns

  // Books/fills ECON-D quality + payload summaries (fast-stream cadence).
  // The ECON-T equivalent lives in HGCalTriggerWorker.
  // TODO: move to endLumi once ECON-D/T per-LS updates land.
  class HGCalQualityWorker : public HGCalDQMWorkerBase {
  public:
    HGCalQualityWorker(std::string folderRoot,
                       ErrorSummarizer& errorSummarizer);
    ~HGCalQualityWorker() override = default;

    void book  (DQMStore::IBooker&, HGCalDQMGeometry const&) override;
    void endRun(DQMStore::IBooker&, DQMStore::IGetter&, HGCalDQMGeometry&) override;

  private:
    std::string folderRoot_;
    ErrorSummarizer& error_summarizer_;   // NOT owned; plugin owns

    int findBinByLabel(MonitorElement* me,
                      const std::string& label) const;

    mutable std::unordered_map<
        MonitorElement*,
        std::unordered_map<std::string, int>>
        label_to_bin_cache_;

    MonitorElement* me_econd_quality_summary_ = nullptr;
    std::map<int, std::map<int, MonitorElement*>>   econdQualityLayer_;
    std::map<int, std::map<int, MonitorElement*>>   econdPayloadLayer_;

    // Fast-stream ECON-D hex plots: plotKey -> layer -> TH2Poly.
    // Keys: "econdQuality", "avgPayload", "stdPayload".
    std::map<std::string, std::map<int, MonitorElement*>> hexPlotsFastStream_;

    const std::vector<std::string> hexPlotsFastStreamKey_ = {
        "econdQuality", "avgPayload", "stdPayload"};
    std::map<int, std::string> endCapKey_ = {{-1, "Minus"}, {1, "Plus"}};
  };

}}  // hgcal::dqm

#endif
