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
    // // Init parameters
    // thickness_weights_ = conf.getParameter<std::vector<float> >("thicknessWeights");
    // weights_ = conf.getParameter<std::vector<float> >("weights");

    // Load ANN model
    static std::unique_ptr<cms::Ort::ONNXRuntime> onnxRuntimeInstance = std::make_unique<cms::Ort::ONNXRuntime>(modelPath_.c_str());
    onnxSession = onnxRuntimeInstance.get();
  }

  void TracksterInferenceByANN::inputData(const std::vector<reco::CaloCluster>& layerClusters,
                                          std::vector<Trackster>& tracksters) {
    // Prepare data for inference
    std::cout << "[INFO] TracksterInferenceByANN::inputData()" << std::endl;

    // calculate the cluster energy sum & decide whether to run inference for the trackster or not
    std::vector<int> tracksterIndices;
    for (int i = 0; i < static_cast<int>(tracksters.size()); i++) {
      float sumClusterEnergy = 0.;
      for (const unsigned int &vertex : tracksters[i].vertices()) {
        sumClusterEnergy += static_cast<float>(layerClusters[vertex].energy());
        if (sumClusterEnergy >= eidMinClusterEnergy_) {
          tracksterIndices.push_back(i);
          break; // early stop upon satisfying the criterion
        }
      }
    }

    // set constants
    const unsigned int n_features = 16;
    const unsigned int lastCEE(rhtools_.lastLayerEE());

    // reset container
    input_Data.clear();
    input_Data.emplace_back(n_features * tracksterIndices.size(), 0);

    // loop over tracksters
    int batchSize = static_cast<int>(tracksterIndices.size());
    for (int i = 0; i < batchSize; i++) {
      const Trackster &trackster = tracksters[tracksterIndices[i]];

      // initialize variables
      float average_eta = 0.;
      float average_phi = 0.;
      std::vector<float> v_mip_energy;
      std::vector<bool> v_is_ceh;
      std::vector<float> energyPerCellType(NUM_CELL_TYPES, 0.);

      //----------------------------------------------------------------------------------------------------
      // calculate energy deposits in different HGCAL conpartments
      //----------------------------------------------------------------------------------------------------
      auto const &layerClusterIndices = trackster.vertices();
      for(auto const idx : layerClusterIndices) {
          auto const &lc = layerClusters[idx];
          auto key = lc.seed();
          auto layer = rhtools_.getLayerWithOffset(key);
          bool isSci = rhtools_.isScintillator(key);
          int thick = isSci ? -1 : rhtools_.getSiThickIndex(key);

          // compute and store MIP energy and CEH flag
          bool isEE(layer<=lastCEE);
          float nmips = computeNmips(lc.energy(), weights_[layer], isSci, isEE, thick);
          v_is_ceh.push_back(!isEE);
          v_mip_energy.push_back(nmips);

          // compute cell type and accumulate energy
          auto cellType = rhtools_.getCellType(key);
          if (cellType == hgcal::CE_H_SCINT) {
              cellType = (layer < SCINT_LAYER_THRESHOLD) ? CE_H_SCINT_F : CE_H_SCINT_C;
          }
          energyPerCellType.at(cellType) += lc.energy();

          // accumulate eta and phi
          average_eta += lc.eta();
          average_phi += lc.phi();
      }

      // compute features
      float num_lc = static_cast<float>(layerClusterIndices.size());
      average_eta /= num_lc;
      average_phi /= num_lc;

      float totalEnergy = std::accumulate(energyPerCellType.begin(), energyPerCellType.end(), 0.0f); // en_l0 = totalEnergy before correction
      float fcee = (energyPerCellType[0] + energyPerCellType[1] + energyPerCellType[2]) / totalEnergy;
      float cglob = computeCglob(v_mip_energy, v_is_ceh);

      // store features
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

  }

  void TracksterInferenceByANN::runInference(std::vector<Trackster>& tracksters) {
    // Run inference using ANN
    std::cout << "[INFO] TracksterInferenceByANN::runInference()" << std::endl;

    // Print the output
    std::cout << "Input dimensions: " << input_Data.size() << " x "
              << (input_Data.empty() ? 0 : input_Data[0].size()) << std::endl;

    for (size_t i = 0; i < input_Data.size(); ++i) {
        std::cout << "Input " << i << ": ";
        for (size_t j = 0; j < input_Data[i].size(); ++j) {
            std::cout << std::fixed << std::setprecision(6) << input_Data[i][j] << " ";
        }
        std::cout << std::endl;
    }

    /*
    //--------------------------------------------------
    // dummy
    //--------------------------------------------------
    // Generate random input features
    std::vector<float> input_tensor_values(10);  // 10 input features
    for (auto& val : input_tensor_values) {
        val = 1.; //dummy
    }

    // layers = [256,128,64,32]

    // // Print input tensor values
    // std::cout << "Input tensor values:" << std::endl;
    // for (size_t i = 0; i < input_tensor_values.size(); ++i) {
    //     std::cout << "  [" << i << "]: " << input_tensor_values[i] << std::endl;
    // }

    // Define input and output names
    std::vector<std::string> input_names = {"input"};
    std::vector<std::vector<float>> input_tensor = {input_tensor_values};
    // std::vector<std::vector<int64_t>> input_shapes = {{static_cast<int64_t>(input_tensor.size())}};
    // std::vector<std::string> output_names = {"output"};
    // int64_t batch_size = 1;
    // std::vector<std::vector<float>> output = onnxSession->run(input_names, input_tensor, input_shapes, output_names, batch_size);
    std::vector<std::vector<float>> output = onnxSession->run(input_names, input_tensor);

    // Print the output
    std::cout << "Output dimensions: " << output.size() << " x "
              << (output.empty() ? 0 : output[0].size()) << std::endl;

    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "Output " << i << ": ";
        for (size_t j = 0; j < output[i].size(); ++j) {
            std::cout << std::fixed << std::setprecision(6) << output[i][j] << " ";
        }
        std::cout << std::endl;
    }
    */
  }

  float TracksterInferenceByANN::computeNmips(float energy, float weight, bool isSci, bool isEE, int thick) {
    // conver energy back to nmips
    // Erec = weight * nmips / thick_weight <=> nmips = Erec * thick_weight / weight; // weights are in keV, energy in GeV
    // https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/src/HGCRecHitAnalyzer.cc?ref_type=heads#L770-782
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
    size_t nhits(hit_en.size());
    if(nhits<min_nhits) return -1.;

    float mean_en = std::accumulate(hit_en.begin(),hit_en.end(),0.0) / nhits;

    //count hits below average or below elim~5MIP
    size_t nbelow_elim(0);
    size_t nbelow_mean(0);
    for(size_t i=0; i<nhits; i++) {
      nbelow_mean += (hit_en[i]<mean_en);
      float elim(is_ceh[i]?elim_ceh:elim_cee);
      nbelow_elim += (hit_en[i]<elim);
    }
    if(nbelow_mean==0) return -1.;

    //cglob ratio
    float cglob=float(nbelow_elim)/float(nbelow_mean);
    cglob=std::min(cglob_max,std::max(cglob_min,cglob));

    return cglob;
  }

  // Method to fill parameter set description for configuration
  void TracksterInferenceByANN::fillPSetDescription(edm::ParameterSetDescription& iDesc) {
    iDesc.add<int>("algo_verbosity", 0);
    iDesc.add<edm::FileInPath>("onnxModelPath", edm::FileInPath("RecoHGCal/TICL/data/simple_net.onnx"))
      ->setComment("Path to PyTorch ANN model in ONNX format");
    iDesc.add<double>("eid_min_cluster_energy", 1.0);
  }
}  // namespace ticl
