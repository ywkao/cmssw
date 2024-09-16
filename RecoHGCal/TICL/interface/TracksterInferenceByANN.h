#ifndef RecoHGCal_TICL_TracksterInferenceByANN_H__
#define RecoHGCal_TICL_TracksterInferenceByANN_H__

#include <numeric>
#include <vector>

#include "RecoHGCal/TICL/interface/TracksterInferenceAlgoBase.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "PhysicsTools/ONNXRuntime/interface/ONNXRuntime.h"

namespace ticl {
  class TracksterInferenceByANN : public TracksterInferenceAlgoBase {
  public:
    explicit TracksterInferenceByANN(const edm::ParameterSet& conf, edm::ConsumesCollector&& cc);
    void inputData(const std::vector<Trackster>& tracksters, const std::vector<reco::CaloCluster>& layerClusters) override;
    void runInference(std::vector<Trackster>& tracksters) override;
    static void fillPSetDescription(edm::ParameterSetDescription& iDesc);

  private:
    float computeNmips(float energy, float weight, bool isSci, bool isEE, int thick);
    float computeCglob(const std::vector<float> &hit_en, const std::vector<bool> &is_ceh,
                       float elim_cee=5., float elim_ceh=5., size_t min_nhits=3, float cglob_min=0., float cglob_max=2.);

    const cms::Ort::ONNXRuntime* onnxSession;
    const std::string modelPath_;
    const double eidMinClusterEnergy_;
    // std::vector<float> thickness_weights_;
    // std::vector<float> weights_;

    // input data
    int batchSize;
    std::vector<int> tracksterIndices;
    std::vector<std::vector<int64_t>> input_shapes;
    std::vector<std::vector<float>> input_Data;

    // patch rhtool
    const int CE_H_SCINT_F = 9;
    const int CE_H_SCINT_C = 10;
    const unsigned int NUM_CELL_TYPES = 11;
    const unsigned int SCINT_LAYER_THRESHOLD = 37;

    std::vector<float> thickness_weights_ = {0.75, 0.76, 0.75, 0.85, 0.85, 0.84, 0.69};
    std::vector<float> weights_ = {0.0, 9.205, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13,
                                   11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 13.2, 13.2,
                                   13.2, 13.2, 13.2, 13.2, 13.2, 13.2, 35.745, 59.665, 60.7, 60.7,
                                   60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 71.89, 83.08, 83.255,
                                   83.52, 83.61, 83.61, 83.61, 83.61, 83.61, 83.61, 83.61}; // keV

    //--------------------------------------------------
    // Note
    //--------------------------------------------------
    // CE_E_120     = 0, CE_E_200     = 1, CE_E_300   = 2,
    // CE_H_120_F   = 3, CE_H_200_F   = 4, CE_H_300_F = 5,
    // CE_H_120_C   = 6, CE_H_200_C   = 7, CE_H_300_C = 8,
    // CE_H_SCINT_F = 9, CE_H_SCINT_C = 10, EnumSize  = 11
    // 26 CEE + 10 CEH with fine Scin. + 11 CEH with coarse Scin.

    //--------------------------------------------------
    // Description
    //--------------------------------------------------
    // // https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/src/HGCRecHitAnalyzer.cc?ref_type=heads#L770-782
    // std::vector<float> default_thickness_weights = {0.75, 0.76, 0.75, 0.85, 0.85, 0.84, 0.69};

    // // https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/test/hgcrechitanalyzer_cfg.py?ref_type=heads#L65
    // // from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import calcWeights,weightsPerLayer_V16
    // std::vector<float> default_weights = {0.0, 9.205, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13,
    //                                        11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 13.2, 13.2,
    //                                        13.2, 13.2, 13.2, 13.2, 13.2, 13.2, 35.745, 59.665, 60.7, 60.7,
    //                                        60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 71.89, 83.08, 83.255,
    //                                        83.52, 83.61, 83.61, 83.61, 83.61, 83.61, 83.61, 83.61}; // keV

    // iDesc.add<std::vector<float>>("thicknessWeights", default_thickness_weights)
    //   ->setComment("Weights for different thicknesses");
    // iDesc.add<std::vector<float>>("weights", default_weights)
    //   ->setComment("Weights per layer (in keV)");
  };
}  // namespace ticl

#endif
