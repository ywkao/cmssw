#ifndef RecoHGCal_TICL_TracksterInferenceByANN_H__
#define RecoHGCal_TICL_TracksterInferenceByANN_H__

#include "RecoHGCal/TICL/interface/TracksterInferenceAlgoBase.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
// #include "PhysicsTools/TensorFlow/interface/TensorFlow.h"

#include "DataFormats/CaloRecHit/interface/CaloCluster.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/RecHitTools.h"
#include "PhysicsTools/ONNXRuntime/interface/ONNXRuntime.h"

namespace ticl {
  class TracksterInferenceByANN : public TracksterInferenceAlgoBase {
  public:
    explicit TracksterInferenceByANN(const edm::ParameterSet& conf);
    void inputData(const std::vector<Trackster>& tracksters) override;
    // void inputData(const std::vector<reco::CaloCluster> &layerClusters, const std::vector<Trackster>& tracksters) override;
    void runInference(std::vector<Trackster>& tracksters) override;

    // static void fillPSetDescription(edm::ParameterSetDescription& desc);

  private:
    // tensorflow::Session* session_;
    const cms::Ort::ONNXRuntime* onnxSession;
    // const float eidMinClusterEnergy_;
    hgcal::RecHitTools rhtools_;
  };
}  // namespace ticl

#endif
