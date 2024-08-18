#include "RecoHGCal/TICL/interface/TracksterInferenceAlgoFactory.h"
#include "RecoHGCal/TICL/interface/TracksterInferenceByANN.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

namespace ticl {

  TracksterInferenceByANN::TracksterInferenceByANN(const edm::ParameterSet& conf)
    : TracksterInferenceAlgoBase(conf)
      // eidMinClusterEnergy_(conf.getParameter<double>("eid_min_cluster_energy"))
  {
    // Load ANN model
  }

  // void TracksterInferenceByANN::fillPSetDescription(edm::ParameterSetDescription& desc) {
  //   desc.add<int>("algo_verbosity", 0);
  //   desc.add<double>("eid_min_cluster_energy", 1.);
  // }

  // void TracksterInferenceByANN::inputData(const std::vector<reco::CaloCluster> &layerClusters, const std::vector<Trackster>& tracksters) {
  void TracksterInferenceByANN::inputData(const std::vector<Trackster>& tracksters) {
    // Prepare data for inference
    
    std::vector<int> tracksterIndices;
    for (int i = 0; i < static_cast<int>(tracksters.size()); i++) {

      float sumClusterEnergy = 0.;
      for (const unsigned int &vertex : tracksters[i].vertices()) {
        sumClusterEnergy += static_cast<float>(layerClusters[vertex].energy());

        // // set default values & set early stop criterion if clusters are too many
        // if (sumClusterEnergy >= eidMinClusterEnergy_) {
        //   tracksters[i].setRegressedEnergy(0.f);
        //   tracksters[i].zeroProbabilities();
        //   tracksterIndices.push_back(i);
        //   break;
        // }
      }
    }

    // // do nothing when no trackster passes the selection
    // int batchSize = static_cast<int>(tracksterIndices.size());
    // if (batchSize == 0) { return; }

    // // fill input tensor
    // for (int i = 0; i < batchSize; i++) {
    //   const Trackster &trackster = tracksters[tracksterIndices[i]];

    //   //----------------------------------------------------------------------------------------------------
    //   // check energy deposit in different HGCAL conpartments
    //   //----------------------------------------------------------------------------------------------------
    //   // CE_E_120 = 0, CE_E_200 = 1, CE_E_300 = 2, CE_H_120_F = 3, CE_H_200_F = 4, CE_H_300_F = 5, CE_H_120_C = 6, CE_H_200_C = 7, CE_H_300_C = 8, CE_H_SCINT = 9, EnumSize = 10
    //   std::vector<float> energyPerCellType(10, 0.); // 10 cell types
    //   auto const &layerClusterIndices = trackster.vertices();
    //   for(auto const idx : layerClusterIndices) {
    //       auto const &lc = layerClusters[idx];
    //       energyPerCellType.at(rhtools_.getCellType(lc.seed())) += lc.energy();
    //   }
    //   std::cout << "[DEBUG] energyPerCellType = ";
    //   for(unsigned int j = 0; j<energyPerCellType.size(); ++j) std::cout << energyPerCellType[j] << " ";
    //   std::cout << std::endl;
    //   //----------------------------------------------------------------------------------------------------

    //   // per layer, we only consider the first eidNClusters_ clusters in terms of energy, so in order
    //   // to avoid creating large / nested structures to do the sorting for an unknown number of total
    //   // clusters, create a sorted list of layer cluster indices to keep track of the filled clusters

    //   // std::vector<int> clusterIndices(trackster.vertices().size());
    //   // for (int k = 0; k < (int)trackster.vertices().size(); k++) {
    //   //   clusterIndices[k] = k;
    //   // }
    //   // sort(clusterIndices.begin(), clusterIndices.end(), [&layerClusters, &trackster](const int &a, const int &b) {
    //   //   return layerClusters[trackster.vertices(a)].energy() > layerClusters[trackster.vertices(b)].energy();
    //   // });

    //   // // keep track of the number of seen clusters per layer
    //   // std::vector<int> seenClusters(eidNLayers_);

    //   // // loop through clusters by descending energy
    //   // for (const int &k : clusterIndices) {
    //   //   // get features per layer and cluster and store the values directly in the input tensor
    //   //   const reco::CaloCluster &cluster = layerClusters[trackster.vertices(k)];
    //   //   int j = rhtools_.getLayerWithOffset(cluster.hitsAndFractions()[0].first) - 1;
    //   //   if (j < eidNLayers_ && seenClusters[j] < eidNClusters_) {
    //   //     // get the pointer to the first feature value for the current batch, layer and cluster
    //   //     // float *features = &input.tensor<float, 4>()(i, j, seenClusters[j], 0);
    //   //     int index1 = 0;	
    //   //     int index2 = (i * eidNLayers_ + j) * eidNClusters_ + seenClusters[j];
    //   //     float *features = &inputData[index1][index2];
    //   //     // fill features
    //   //     *(features++) = float(cluster.energy() / float(trackster.vertex_multiplicity(k)));
    //   //     *(features++) = float(std::abs(cluster.eta()));
    //   //     *(features) = float(cluster.phi());

    //   //     // increment seen clusters
    //   //     seenClusters[j]++;
    //   //   }
    //   // }
    // } // end of loop over tracksters

  }

  void TracksterInferenceByANN::runInference(std::vector<Trackster>& tracksters) {
    // Run inference using ANN

    // std::vector<std::vector<float> > outputTensors;
    // outputTensors = onnxSession->run(inputNames, inputData, input_shapes, outNames, batchSize);
    // std::cout<< "size of outputTensors = " << outputTensors.size() << std::endl;
  }

}

// Define this as a plug-in
#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_EDM_PLUGIN(TracksterInferenceAlgoFactory, ticl::TracksterInferenceByANN, "TracksterInferenceByANN");
