#include "RecoHGCal/TICL/interface/TracksterInferenceAlgoFactory.h"
#include "RecoHGCal/TICL/interface/TracksterInferenceByANN.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

namespace ticl {

  TracksterInferenceByANN::TracksterInferenceByANN(const edm::ParameterSet& conf, edm::ConsumesCollector&& cc)
    : TracksterInferenceAlgoBase(conf, std::move(cc)),
      modelPath_(conf.getParameter<edm::FileInPath>("onnxModelPath").fullPath()),
      eidMinClusterEnergy_(conf.getParameter<double>("eid_min_cluster_energy"))
  {
    // Init parameters
    thickness_weights_ = conf.getParameter<std::vector<double> >("thicknessWeights");
    weights_ = conf.getParameter<std::vector<double> >("weights");

    // Load ANN model
    static std::unique_ptr<cms::Ort::ONNXRuntime> onnxRuntimeInstance = std::make_unique<cms::Ort::ONNXRuntime>(modelPath_.c_str());
    onnxSession_ = onnxRuntimeInstance.get();
  }

  void TracksterInferenceByANN::inputData(const std::vector<reco::CaloCluster>& layerClusters,
                                          std::vector<Trackster>& tracksters) {
    // Prepare data for inference
    // Calculate the cluster energy sum & decide whether to run inference for the trackster or not
    tracksterIndices.clear(); // Clear previous indices
    for (int i = 0; i < static_cast<int>(tracksters.size()); i++) {
      float sumClusterEnergy = 0.;
      for (const unsigned int& vertex : tracksters[i].vertices()) {
        sumClusterEnergy += static_cast<float>(layerClusters[vertex].energy());
        if (sumClusterEnergy >= eidMinClusterEnergy_) {
          tracksters[i].setRegressedEnergy(0.f); // Set regressed energy to 0
          tracksters[i].zeroProbabilities(); // Zero out probabilities
          tracksterIndices.push_back(i); // Add index to the list
          break;
        }
      }
   }

    // Prepare input shapes and data for inference
    batchSize = static_cast<int>(tracksterIndices.size());
    if (batchSize == 0) return; // Exit if no tracksters

    // Set constants & reset containers
    const unsigned int lastCEE(rhtools_.lastLayerEE());

    std::vector<int64_t> inputShape = {MAX_ENTRIES, n_features};
    input_shapes = {inputShape};

    input_Data.clear();
    input_Data.emplace_back(n_features * MAX_ENTRIES, 0); // Padding for a fixed input size

    // Loop over tracksters
    for (size_t i = 0; i < batchSize; i++) {
      const Trackster &trackster = tracksters[tracksterIndices[i]];

      // Initialize variables
      float average_eta = 0.;
      float average_phi = 0.;
      std::vector<float> v_mip_energy;
      std::vector<bool> v_is_ceh;
      std::vector<float> energyPerCellType(NUM_CELL_TYPES, 0.);

      //----------------------------------------------------------------------------------------------------
      // Calculate energy deposits in different HGCAL conpartments
      //----------------------------------------------------------------------------------------------------
      auto const &layerClusterIndices = trackster.vertices();
      for(auto const idx : layerClusterIndices) {
          auto const &lc = layerClusters[idx];
          auto key = lc.seed();
          auto layer = rhtools_.getLayerWithOffset(key);
          bool isSci = rhtools_.isScintillator(key);
          int thick = isSci ? -1 : rhtools_.getSiThickIndex(key);

          // Compute and store MIP energy and CEH flag
          bool isEE(layer<=lastCEE);
          float nmips = computeNmips(lc.energy(), weights_[layer], isSci, isEE, thick);
          v_is_ceh.push_back(!isEE);
          v_mip_energy.push_back(nmips);

          // Compute cell type and accumulate energy
          auto cellType = rhtools_.getCellType(key);
          if (cellType == hgcal::CE_H_SCINT) {
              cellType = (layer < SCINT_LAYER_THRESHOLD) ? CE_H_SCINT_F : CE_H_SCINT_C;
          }
          auto feature_order = get_feature_order(cellType);
          energyPerCellType.at(feature_order) += lc.energy();

          // Accumulate eta and phi
          average_eta += lc.eta();
          average_phi += lc.phi();
      }

      // Compute features
      float num_lc = static_cast<float>(layerClusterIndices.size());
      average_eta /= num_lc;
      average_phi /= num_lc;

      float totalEnergy = std::accumulate(energyPerCellType.begin(), energyPerCellType.end(), 0.0f); // en_l0 = totalEnergy before correction
      float fcee = (energyPerCellType[0] + energyPerCellType[1] + energyPerCellType[2]) / totalEnergy;
      float cglob = computeCglob(v_mip_energy, v_is_ceh);

      // Store features
      auto index = i*n_features;
      input_Data[0][index] = totalEnergy;
      input_Data[0][index+1] = average_eta;
      input_Data[0][index+2] = average_phi;
      input_Data[0][index+3] = fcee;
      input_Data[0][index+4] = cglob;
      for(unsigned int j = 0; j<energyPerCellType.size(); ++j) {
          input_Data[0][index+5+j] = energyPerCellType[j];
      }
    } // end of looping tracksters
  } // end of inputData

  void TracksterInferenceByANN::runInference(std::vector<Trackster>& tracksters) {
    // Run inference using ANN
    if (batchSize == 0) return; // Exit if no batch

    // Define input and output names for inference
    std::vector<std::string> inputNames = {"x"};
    std::vector<std::string> outNames  = {"491", "492"}; // ratio and sigma

    // Run prediction
    std::vector<std::vector<float> > outputTensors;
    outputTensors = onnxSession_->run(inputNames, input_Data, input_shapes, outNames, MAX_ENTRIES); // batchSize

    // Derive regressed energy
    for (size_t i = 0; i < batchSize; i++) {
      Trackster &trackster = tracksters[tracksterIndices[i]];
      float ratio = outputTensors[0][i];
      float sigma = outputTensors[1][i];
      unsigned int index = i*n_features;
      float raw_total_energy = input_Data[0][index];
      float regressed_energy = ratio * raw_total_energy; 
      trackster.setRegressedEnergy(regressed_energy);
    }
  } // end of runInference

  float TracksterInferenceByANN::computeNmips(float energy, float weight, bool isSci, bool isEE, int thick) {
    // Conver energy back to nmips
    // Erec = weight * nmips / thick_weight <=> nmips = Erec * thick_weight / weightm where weights are in keV, energy in GeV
    // Reference: https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/src/HGCRecHitAnalyzer.cc?ref_type=heads#L770-782
    float mipSF(1./(weight*1e-3));
    if(!isSci) {
      size_t idx(isEE ? 0 : 3);
      if(thick==0) idx+=0;
      else if(thick==1) idx+=1;
      else idx+=2;
      mipSF *= thickness_weights_[idx];
    } else {
      mipSF *= thickness_weights_[6];
    }
    float nmips(energy*mipSF);
    return nmips;
  }

  float TracksterInferenceByANN::computeCglob(const std::vector<float> &hit_en, const std::vector<bool> &is_ceh,
                                              float elim_cee, float elim_ceh, size_t min_nhits, float cglob_min, float cglob_max) {
    // Compute a variable value associated with global correlation
    // Reference: https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/src/HGCRecHitAnalyzer.cc?ref_type=heads#L631
    size_t nhits(hit_en.size());
    if(nhits<min_nhits) return -1.;

    float mean_en = std::accumulate(hit_en.begin(),hit_en.end(),0.0) / nhits;

    // Count hits below average or below elim~5MIP
    size_t nbelow_elim(0);
    size_t nbelow_mean(0);
    for(size_t i=0; i<nhits; i++) {
      nbelow_mean += (hit_en[i]<mean_en);
      float elim(is_ceh[i]?elim_ceh:elim_cee);
      nbelow_elim += (hit_en[i]<elim);
    }
    if(nbelow_mean==0) return -1.;

    // cglob ratio
    float cglob=float(nbelow_elim)/float(nbelow_mean);
    cglob=std::min(cglob_max,std::max(cglob_min,cglob));

    return cglob;
  }

  int TracksterInferenceByANN::get_feature_order(int cell_type) {
    // A mapping between the cell types in rhtools & the feature orders in ANN models
    switch (cell_type) {
        case 0: return 0;  // CE_E_120 -> sumen_CEESi100
        case 1: return 1;  // CE_E_200 -> sumen_CEESi200
        case 2: return 2;  // CE_E_300 -> sumen_CEESi300
        case 3: return 6;  // CE_H_120_F -> sumen_CEHfineSi100
        case 4: return 7;  // CE_H_200_F -> sumen_CEHfineSi200
        case 5: return 8;  // CE_H_300_F -> sumen_CEHfineSi300
        case 6: return 3;  // CE_H_120_C -> sumen_CEHcoarseSi100
        case 7: return 4;  // CE_H_200_C -> sumen_CEHcoarseSi200
        case 8: return 5;  // CE_H_300_C -> sumen_CEHcoarseSi300
        case 9: return 10; // CE_H_SCINT_F -> sumen_CEHScifine
        case 10: return 9; // CE_H_SCINT_C -> sumen_CEHScicoarse
        default:
            throw std::invalid_argument("Invalid cell type");
    }
  }

  void TracksterInferenceByANN::fillPSetDescription(edm::ParameterSetDescription& iDesc) {
    // Method to fill parameter set description for configuration
    iDesc.add<int>("algo_verbosity", 0);

    // Path to an onnx model, tk = trackster
    iDesc.add<edm::FileInPath>("onnxModelPath", edm::FileInPath("RecoHGCal/TICL/data/ticlv5/onnx_models/hadron_energy_regression/semiparametric_model_tk_pca_2024Apr29.onnx"))
      ->setComment("Path to PyTorch ANN model in ONNX format");

    // Energy threshold to select a trackster
    iDesc.add<double>("eid_min_cluster_energy", 1.0);

    // Reference: https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/src/HGCRecHitAnalyzer.cc?ref_type=heads#L770-782
    std::vector<double> default_thickness_weights = {0.75, 0.76, 0.75, 0.85, 0.85, 0.84, 0.69};

    // Reference: https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/test/hgcrechitanalyzer_cfg.py?ref_type=heads#L65
    // from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import calcWeights,weightsPerLayer_V16
    std::vector<double> default_weights = {0.0, 9.205, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13,
                                           11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 11.13, 13.2, 13.2,
                                           13.2, 13.2, 13.2, 13.2, 13.2, 13.2, 35.745, 59.665, 60.7, 60.7,
                                           60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 60.7, 71.89, 83.08, 83.255,
                                           83.52, 83.61, 83.61, 83.61, 83.61, 83.61, 83.61, 83.61}; // keV

    iDesc.add<std::vector<double>>("thicknessWeights", default_thickness_weights)
      ->setComment("Weights for different thicknesses");
    iDesc.add<std::vector<double>>("weights", default_weights)
      ->setComment("Weights per layer (in keV)");
  }
}  // namespace ticl
