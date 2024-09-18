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
    void inputData(const std::vector<reco::CaloCluster>& layerClusters, std::vector<Trackster>& tracksters) override;
    void runInference(std::vector<Trackster>& tracksters) override;
    static void fillPSetDescription(edm::ParameterSetDescription& iDesc);

  private:
    int get_feature_order(int cell_type);
    float computeNmips(float energy, float weight, bool isSci, bool isEE, int thick);
    float computeCglob(const std::vector<float> &hit_en, const std::vector<bool> &is_ceh,
                       float elim_cee=5., float elim_ceh=5., size_t min_nhits=3, float cglob_min=0., float cglob_max=2.);
    // source of the default values: https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/src/HGCRecHitAnalyzer.cc?ref_type=heads#L631

    const cms::Ort::ONNXRuntime* onnxSession_;
    const std::string modelPath_;
    const double eidMinClusterEnergy_;
    std::vector<double> thickness_weights_;
    std::vector<double> weights_;

    // input data
    const unsigned int MAX_ENTRIES = 50; // allowed max number of tracksters for padding input data
    const unsigned int n_features = 16;

    unsigned int batchSize;
    std::vector<int> tracksterIndices;
    std::vector<std::vector<int64_t>> input_shapes;
    std::vector<std::vector<float>> input_Data;

    // patch rhtool
    const int CE_H_SCINT_F = 9;
    const int CE_H_SCINT_C = 10;
    const unsigned int NUM_CELL_TYPES = 11;
    const unsigned int SCINT_LAYER_THRESHOLD = 37;

    //--------------------------------------------------
    // NUM_CELL_TYPES & SCINT_LAYER_THRESHOLD
    //--------------------------------------------------
    // CE_E_120     = 0, CE_E_200     = 1, CE_E_300   = 2,
    // CE_H_120_F   = 3, CE_H_200_F   = 4, CE_H_300_F = 5,
    // CE_H_120_C   = 6, CE_H_200_C   = 7, CE_H_300_C = 8,
    // CE_H_SCINT_F = 9, CE_H_SCINT_C = 10, EnumSize  = 11
    // 26 CEE + 10 CEH with fine Scin. + 11 CEH with coarse Scin.
  };
}  // namespace ticl

#endif
