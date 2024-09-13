#include "RecoHGCal/TICL/interface/TracksterInferenceAlgoFactory.h"
#include "RecoHGCal/TICL/interface/TracksterInferenceByANN.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

namespace ticl {

  TracksterInferenceByANN::TracksterInferenceByANN(const edm::ParameterSet& conf)
    : TracksterInferenceAlgoBase(conf) {
    // Load ANN model
  }

  void TracksterInferenceByANN::inputData(const std::vector<Trackster>& tracksters, const std::vector<reco::CaloCluster>& layerClusters) {
    // Prepare data for inference
    std::cout << "[INFO] TracksterInferenceByANN::inputData()" << std::endl;

    //calculate the cluster energy sum & decide whether to run inference for the trackster or not
    std::vector<int> tracksterIndices;
    for (int i = 0; i < static_cast<int>(tracksters.size()); i++) {
      float sumClusterEnergy = 0.;
      for (const unsigned int &vertex : tracksters[i].vertices()) {
        sumClusterEnergy += static_cast<float>(layerClusters[vertex].energy());
        if (sumClusterEnergy >= eidMinClusterEnergy_) {
          tracksterIndices.push_back(i);
          break; // there might be many clusters, so try to stop early
        }
      }
    }

    int batchSize = static_cast<int>(tracksterIndices.size());
    for (int i = 0; i < batchSize; i++) {
      const Trackster &trackster = tracksters[tracksterIndices[i]];
      //----------------------------------------------------------------------------------------------------
      // check energy deposit in different HGCAL conpartments
      //----------------------------------------------------------------------------------------------------
      // CE_E_120 = 0, CE_E_200 = 1, CE_E_300 = 2, CE_H_120_F = 3, CE_H_200_F = 4, CE_H_300_F = 5, CE_H_120_C = 6, CE_H_200_C = 7, CE_H_300_C = 8, CE_H_SCINT = 9, EnumSize = 10
      // float en_l0 = trackster.raw_energy(); // returns zero
      float average_eta = 0.;
      float average_phi = 0.;
      std::vector<float> v_mip_energy;
      std::vector<bool> v_is_ceh;
      std::vector<float> energyPerCellType(11, 0.); // 11 cell types
      auto const &layerClusterIndices = trackster.vertices();
      for(auto const idx : layerClusterIndices) {
          auto const &lc = layerClusters[idx];
          /*
          auto key = lc.seed();
          auto layer = rhtools_.getLayerWithOffset(key);
          bool isSci = rhtools_.isScintillator(key);
          int thick = isSci ? -1 : rhtools_.getSiThickIndex(key);
          unsigned int lastCEE(rhtools_.lastLayerEE());
          bool isEE(layer<=lastCEE);

          // conver energy back to nmips
          // Erec = weight * nmips / thick_weight <=> nmips = Erec * thick_weight / weight; // weights are in keV, energy in GeV
          // https://gitlab.cern.ch/psilva/hgcalhadronicenergyscalestudies/-/blob/master/src/HGCRecHitAnalyzer.cc?ref_type=heads#L770-782
          float mipSF(1./(weights_[layer]*1e-3));
          if(!isSci) {
            size_t idx(isEE ? 0 : 3);
            if(thick==0) idx+=0;
            else if(thick==1) idx+=1;
            else idx+=2;
            mipSF *= thickness_weights_[idx];
          } else {
            mipSF *= thickness_weights_[6];
          }
          float nmips(lc.energy()*mipSF);

          auto cellType = rhtools_.getCellType(key);
          cellType = (cellType==hgcal::CE_H_SCINT) ? ((layer<37) ? CE_H_SCINT_F : CE_H_SCINT_C) : cellType; // 26 + 10 + 11
          energyPerCellType.at(cellType) += lc.energy();
          v_mip_energy.push_back(nmips);
          v_is_ceh.push_back(!isEE);
          */
          average_eta += lc.eta();
          average_phi += lc.phi();
      }

      float num_lc = (float) layerClusterIndices.size();
      average_eta /= num_lc;
      average_phi /= num_lc;

      std::cout << "[INFO] num_lc "
                << "= " << num_lc
                << "; nmips = ";
      for(unsigned int j = 0; j<v_mip_energy.size(); ++j) std::cout << v_mip_energy[j] << " ";
      std::cout << std::endl;

      float totalEnergy = std::accumulate(energyPerCellType.begin(), energyPerCellType.end(), 0.0f); // en_l0 = totalEnergy before correction
      float fcee = (energyPerCellType[0] + energyPerCellType[1] + energyPerCellType[2]) / totalEnergy;
      float cglob = computeCglob(v_mip_energy, v_is_ceh);

      std::cout << "[INFO] en_l0, eta, phi, fcee, cglob "
                << "= " << totalEnergy
                << ", " << average_eta
                << ", " << average_phi
                << ", " << fcee
                << ", " << cglob
                << "; energyPerCellType = ";
      for(unsigned int j = 0; j<energyPerCellType.size(); ++j) std::cout << energyPerCellType[j] << " ";
      std::cout << std::endl;

      //----------------------------------------------------------------------------------------------------

      // std::vector<int> clusterIndices(trackster.vertices().size());
      // for (int k = 0; k < (int)trackster.vertices().size(); k++) {
      //   clusterIndices[k] = k;
      // }
      // sort(clusterIndices.begin(), clusterIndices.end(), [&layerClusters, &trackster](const int &a, const int &b) {
      //   return layerClusters[trackster.vertices(a)].energy() > layerClusters[trackster.vertices(b)].energy();
      // });

      // // keep track of the number of seen clusters per layer
      // std::vector<int> seenClusters(eidNLayers_);

      // // loop through clusters by descending energy
      // for (const int &k : clusterIndices) {
      //   // get features per layer and cluster and store the values directly in the input tensor
      //   const reco::CaloCluster &cluster = layerClusters[trackster.vertices(k)];
      //   int j = rhtools_.getLayerWithOffset(cluster.hitsAndFractions()[0].first) - 1;
      //   if (j < eidNLayers_ && seenClusters[j] < eidNClusters_) {
      //     // get the pointer to the first feature value for the current batch, layer and cluster
      //     // float *features = &input.tensor<float, 4>()(i, j, seenClusters[j], 0);
      //     int index1 = 0;	
      //     int index2 = (i * eidNLayers_ + j) * eidNClusters_ + seenClusters[j];
      //     float *features = &inputData[index1][index2];
      //     // fill features
      //     *(features++) = float(cluster.energy() / float(trackster.vertex_multiplicity(k)));
      //     *(features++) = float(std::abs(cluster.eta()));
      //     *(features) = float(cluster.phi());

      //     // increment seen clusters
      //     seenClusters[j]++;
      //   }
      // }
    }
  }

  void TracksterInferenceByANN::runInference(std::vector<Trackster>& tracksters) {
    // Run inference using ANN
    std::cout << "[INFO] TracksterInferenceByANN::runInference()" << std::endl;
  }

  float TracksterInferenceByANN::computeCglob(const std::vector<float> &hit_en,
                                        const std::vector<bool> &is_ceh,
                                        float elim_cee, float elim_ceh, size_t min_nhits,
                                        float cglob_min, float cglob_max)
  {
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
}
