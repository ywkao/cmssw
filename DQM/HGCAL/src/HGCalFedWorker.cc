#include "DQM/HGCAL/interface/HGCalFedWorker.h"

#include "DQM/HGCAL/interface/HGCalDQMGeometry.h"

namespace hgcal {
  namespace dqm {

    HGCalFedWorker::HGCalFedWorker(std::string folderRoot) : folderRoot_(std::move(folderRoot)) {}

    void HGCalFedWorker::endRun(DQMStore::IBooker& ibooker, DQMStore::IGetter& igetter, HGCalDQMGeometry& /*geom*/) {
      // project all FED payload
      MonitorElement* me = igetter.get(folderRoot_ + "/FED/fedPayload");

      if (!me || !(me->getTH2F())) {
        edm::LogWarning("HGCalFedWorker") << "Could not find fedPayload histogram";
        return;
      }

      // 1D payload distribution using TH2F::ProjectionY()
      TH1* me_proj = me->getTH2F()->ProjectionY();
      ibooker.setCurrentFolder(folderRoot_ + "/FED");
      me_fed_payload_th1d_ = ibooker.book1D("fed_payload_distribution",
                                            "FED Payload Distribution;Payload;Counts",
                                            me_proj->GetNbinsX(),
                                            me_proj->GetXaxis()->GetXmin(),
                                            me_proj->GetXaxis()->GetXmax());
      me_fed_payload_th1d_->getTH1()->Add(me_proj);
      delete me_proj;
    }

  }  // namespace dqm
}  // namespace hgcal
