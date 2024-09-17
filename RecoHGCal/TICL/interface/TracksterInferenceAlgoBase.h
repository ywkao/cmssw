// Author: Felice Pantaleo - felice.pantaleo@cern.ch
// Date: 07/2024

#ifndef RecoHGCal_TICL_TracksterInferenceAlgo_H__
#define RecoHGCal_TICL_TracksterInferenceAlgo_H__

#include <vector>
#include "DataFormats/HGCalReco/interface/Trackster.h"
#include "DataFormats/CaloRecHit/interface/CaloCluster.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/RecHitTools.h"

namespace ticl {
  class TracksterInferenceAlgoBase {
  public:
    explicit TracksterInferenceAlgoBase(const edm::ParameterSet& conf, edm::ConsumesCollector&& cc)
      : algo_verbosity_(conf.getParameter<int>("algo_verbosity"))
    {
      caloGeomToken_ = cc.esConsumes<CaloGeometry, CaloGeometryRecord>();
    }
    virtual ~TracksterInferenceAlgoBase() {}

    virtual void inputData(const std::vector<reco::CaloCluster>& layerClusters, std::vector<Trackster>& tracksters) = 0;
    virtual void runInference(std::vector<Trackster>& tracksters) = 0;
    static void fillPSetDescription(edm::ParameterSetDescription& desc) { desc.add<int>("algo_verbosity", 0); };
    virtual void updateGeometry(const edm::EventSetup& es) {
      const CaloGeometry& geom = es.getData(caloGeomToken_);
      rhtools_.setGeometry(geom);
    }

  protected:
    int algo_verbosity_;
    hgcal::RecHitTools rhtools_;
	edm::ESGetToken<CaloGeometry, CaloGeometryRecord> caloGeomToken_;
  };
}  // namespace ticl

#endif
