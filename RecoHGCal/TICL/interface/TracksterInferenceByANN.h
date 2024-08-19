#ifndef RecoHGCal_TICL_TracksterInferenceByANN_H__
#define RecoHGCal_TICL_TracksterInferenceByANN_H__

#include <numeric>
#include <vector>

#include "RecoHGCal/TICL/interface/TracksterInferenceAlgoBase.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/RecHitTools.h"

namespace ticl {
  class TracksterInferenceByANN : public TracksterInferenceAlgoBase {
  public:
    explicit TracksterInferenceByANN(const edm::ParameterSet& conf);
    void inputData(const std::vector<reco::CaloCluster>& layerClusters, std::vector<Trackster>& tracksters) override;
    void runInference(std::vector<Trackster>& tracksters) override;
    float computeCglob(const std::vector<float> &hit_en, const std::vector<bool> &is_ceh,
                       float elim_cee=5., float elim_ceh=5., size_t min_nhits=3, float cglob_min=0., float cglob_max=2.);

  private:
    const cms::Ort::ONNXRuntime* onnxSession;
    hgcal::RecHitTools rhtools_;

    // patch rhtool
    const int CE_H_SCINT_F = 9;
    const int CE_H_SCINT_C = 10;

    // eidMinClusterEnergy_(conf.getParameter<double>("eid_min_cluster_energy")),
    float eidMinClusterEnergy_ = 1.;

    // https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/src/HGCRecHitAnalyzer.cc?ref_type=heads#L770-782
    std::vector<float> thickness_weights_ = {0.75, 0.76, 0.75, 0.85, 0.85, 0.84, 0.69};

    // https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/test/hgcrechitanalyzer_cfg.py?ref_type=heads#L65
    // from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import calcWeights,weightsPerLayer_V16
    std::vector<float> weights_ = {0.0, 9.205, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 11.129999999999999, 13.2, 13.2, 13.2, 13.2, 13.2, 13.2, 13.2, 13.2, 35.745000000000005, 59.665000000000006, 60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 71.89, 83.08, 83.255, 83.52000000000001, 83.61, 83.61, 83.61, 83.61, 83.61, 83.61, 83.61}; // keV

  };
}  // namespace ticl

#endif
