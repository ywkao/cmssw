#include <memory>
#include <string>
#include <fstream>
#include <iostream>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"

#include "Validation/PlaygroundEDProducer/interface/hgcalhit.h"
#include "Validation/PlaygroundEDProducer/interface/RunningCollection.h"
#include "Validation/PlaygroundEDProducer/interface/LoadCalibrationParameters.h"

// for loading ntuple as temporary input
#include "TBranch.h"
#include "TFile.h"
#include "TTree.h"

//
// class declaration
//

class PlaygroundEDProducer : public edm::stream::EDProducer<> {
public:
  explicit PlaygroundEDProducer(const edm::ParameterSet&);
  ~PlaygroundEDProducer();

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void beginStream(edm::StreamID) override;
  void produce(edm::Event&, const edm::EventSetup&) override;
  void endStream() override;

  virtual void     Init(TTree *tree=0); // in order to load ntuple (temporary input from 2022 testbeam data)
  virtual Long64_t LoadTree(Long64_t entry);

  virtual void     enable_pedestal_subtraction();
  virtual void     enable_cm_subtraction();

  //virtual void beginRun(edm::Run const&, edm::EventSetup const&) override;
  //virtual void endRun(edm::Run const&, edm::EventSetup const&) override;
  //virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;
  //virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;

  // ------------ member data ------------
  std::string folder_;
  TString myTag;
  std::vector<int> calibration_flags;
  std::string csv_file_name;

  TString tag_calibration;
  TString tag_channelId;

  CalibrationParameterLoader calib_loader;
  RunningCollection myRunStatCollection;
  RunningStatistics myRecorder;

  bool flag_perform_pedestal_subtraction;
  bool flag_perform_cm_subtraction;

  int globalChannelId;
  double adc_double;
  double adc_channel_CM;

  //--------------------------------------------------
  // for reading ntuple (temporary)
  //--------------------------------------------------
  TFile          *f1;
  TTree          *t1;

  TTree          *fChain;   //!pointer to the analyzed TTree or TChain
  Int_t           fCurrent; //!current Tree number in a TChain

  // Declaration of leaf types
  Int_t           event;
  Int_t           chip;
  Int_t           half;
  Int_t           channel;
  Int_t           adc;
  Int_t           adcm;
  Int_t           toa;
  Int_t           tot;
  Int_t           totflag;
  Int_t           trigtime;
  Int_t           trigwidth;
  Int_t           corruption;
  Int_t           bxcounter;
  Int_t           eventcounter;
  Int_t           orbitcounter;

  // List of branches
  TBranch        *b_event;
  TBranch        *b_chip;
  TBranch        *b_half;
  TBranch        *b_channel;
  TBranch        *b_adc;
  TBranch        *b_adcm;
  TBranch        *b_toa;
  TBranch        *b_tot;
  TBranch        *b_totflag;
  TBranch        *b_trigtime;
  TBranch        *b_trigwidth;
  TBranch        *b_corruption;
  TBranch        *b_bxcounter;
  TBranch        *b_eventcounter;
  TBranch        *b_orbitcounter;
};

//
// constructors and destructor
//

PlaygroundEDProducer::PlaygroundEDProducer(const edm::ParameterSet& iConfig) :
    folder_(iConfig.getParameter<std::string>("folder")),
    myTag(iConfig.getParameter<std::string>( "DataType" )),
    calibration_flags(iConfig.getParameter<std::vector<int> >( "CalibrationFlags" )),
    csv_file_name(iConfig.getParameter<std::string>( "CalibrationCSVFile" ))
{
  // load trees from beam data / pedestal run
  TString root_beamRun  = "/afs/cern.ch/work/y/ykao/public/forJeremi/beam_run0.root";
  TString root_pedestal = "/afs/cern.ch/work/y/ykao/public/forJeremi/pedestal_run0.root";
  TString input = (myTag=="beam") ? root_beamRun : root_pedestal;
  printf("[INFO] Input rootfile: %s\n", input.Data());

  f1 = new TFile(input, "R");
  t1 = (TTree*) f1->Get("unpacker_data/hgcroc");
  Init(t1); // SetBranchAddress, init variables, etc.

  // determine which calibrations to perform
  if(calibration_flags[0]) enable_pedestal_subtraction();
  if(calibration_flags[1]) enable_cm_subtraction();

  calib_loader.loadParameters(csv_file_name);

  produces<RecHit>();
}

PlaygroundEDProducer::~PlaygroundEDProducer() {}

//
// member functions
//

// ------------ method called to produce the data  ------------
void PlaygroundEDProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  using namespace edm;
  printf("[INFO] test\n");

  //--------------------------------------------------
  // migrate from a standalone c++ code
  //--------------------------------------------------
  Long64_t nentries = fChain->GetEntriesFast();
  int nevent = nentries / 78;

  std::cout << ">>> nentries = " << nentries << std::endl;
  std::cout << ">>> nevent = " << nevent << std::endl;

  // loop over hit
  std::vector<RecHit> hits[nevent];
  int current_half      = 0;
  int recorded_half     = 0;
  double adc_channel_37 = 0.;
  RecHit hit_channel_37;

  Long64_t nbytes = 0, nb = 0;
  for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry); nbytes += nb;

      // reset cm adc when reaching another ROC half
      current_half = half;
      if(current_half != recorded_half) {
          recorded_half = current_half;
          adc_channel_CM = 0.;
      }

      // get detId & mask bad channel
      auto detid = DetectorId( FromRawData(), chip, half, channel );
      globalChannelId = detid.id(); // chip*78+half*39+channel;
      bool is_bad_channel = globalChannelId==146 || globalChannelId==171;
      if(is_bad_channel) continue;

      // convert adc to double
      adc_double = (double) adc;

      // perform pedestal subtraction
      if(flag_perform_pedestal_subtraction) {
          double pedestal = calib_loader.map_pedestals[globalChannelId];
          adc_double -= pedestal;
      }

      // handle cm information after pedestal subtraction
      bool is_cm_channel = (globalChannelId % 39 == 37 || globalChannelId % 39 == 38);
      if(is_cm_channel) {
          // take average of two cm channels in a half
          adc_channel_CM += adc_double / 2.;
      }

      // record adc of ch37 & fill info of ch37 when processing ch38
      if(globalChannelId % 39 == 37) {
          adc_channel_37 = adc_double;
          RecHit hit( event, detid, adc_double, adc_channel_CM, adcm, toa, tot, trigtime );
          hit_channel_37 = hit;
          continue;

      } else if(globalChannelId % 39 == 38) {
          // update CM adc value
          hit_channel_37.set_adc_CM(adc_channel_CM);

          // CM subtraction for channel 37
          if(flag_perform_cm_subtraction) {
              std::vector<double> parameters = calib_loader.map_cm_parameters[globalChannelId-1];
              double slope = parameters[0];
              double intercept = parameters[1];
              double correction = adc_channel_CM*slope + intercept;
              adc_channel_37 -= correction;

              hit_channel_37.set_adc(adc_channel_37);
          }

          // store info of ch37 when process ch38
          iEvent.put( std::make_unique<RecHit>(hit_channel_37) );
      }

      // perform common mode subtraction
      if(flag_perform_cm_subtraction) {
          std::vector<double> parameters = calib_loader.map_cm_parameters[globalChannelId];
          double slope = parameters[0];
          double intercept = parameters[1];
          double correction = adc_channel_CM*slope + intercept;
          adc_double -= correction;
      }

      RecHit hit( event, detid, adc_double, adc_channel_CM, adcm, toa, tot, trigtime );
      iEvent.put( std::make_unique<RecHit>(hit) );
  }

} // end of produce function

// ------------ auxilliary methods  ------------
Long64_t PlaygroundEDProducer::LoadTree(Long64_t entry)
{
    // Set the environment to read one entry
    if (!fChain) return -5;
    Long64_t centry = fChain->LoadTree(entry);
    if (centry < 0) return centry;
    if (fChain->GetTreeNumber() != fCurrent) {
        fCurrent = fChain->GetTreeNumber();
    }
    return centry;
}

void PlaygroundEDProducer::Init(TTree *tree)
{
    if (tree == 0) printf("[ERROR] something goes wrong with input tree\n");

    if (!tree) return;
    fChain = tree;
    fCurrent = -1;
    fChain->SetMakeClass(1);

    fChain->SetBranchAddress("event", &event, &b_event);
    fChain->SetBranchAddress("chip", &chip, &b_chip);
    fChain->SetBranchAddress("half", &half, &b_half);
    fChain->SetBranchAddress("channel", &channel, &b_channel);
    fChain->SetBranchAddress("adc", &adc, &b_adc);
    fChain->SetBranchAddress("adcm", &adcm, &b_adcm);
    fChain->SetBranchAddress("toa", &toa, &b_toa);
    fChain->SetBranchAddress("tot", &tot, &b_tot);
    fChain->SetBranchAddress("totflag", &totflag, &b_totflag);
    fChain->SetBranchAddress("trigtime", &trigtime, &b_trigtime);
    fChain->SetBranchAddress("trigwidth", &trigwidth, &b_trigwidth);
    fChain->SetBranchAddress("corruption", &corruption, &b_corruption);
    fChain->SetBranchAddress("bxcounter", &bxcounter, &b_bxcounter);
    fChain->SetBranchAddress("eventcounter", &eventcounter, &b_eventcounter);
    fChain->SetBranchAddress("orbitcounter", &orbitcounter, &b_orbitcounter);

    flag_perform_pedestal_subtraction = false;
    flag_perform_cm_subtraction = false;

    tag_calibration = "";
    tag_channelId = "_channel_7";
}

void PlaygroundEDProducer::enable_pedestal_subtraction() { flag_perform_pedestal_subtraction = true; tag_calibration = "_ped_subtracted"; }

void PlaygroundEDProducer::enable_cm_subtraction() { flag_perform_cm_subtraction = true; tag_calibration = "_cm_subtracted"; }

// ------------ method called once each stream before processing any runs, lumis or events  ------------
void PlaygroundEDProducer::beginStream(edm::StreamID) {
  // please remove this method if not needed
}

// ------------ method called once each stream after processing all runs, lumis and events  ------------
void PlaygroundEDProducer::endStream() {
  // please remove this method if not needed
}

// ------------ method called when starting to processes a run  ------------
/*
void
PlaygroundEDProducer::beginRun(edm::Run const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when ending the processing of a run  ------------
/*
void
PlaygroundEDProducer::endRun(edm::Run const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when starting to processes a luminosity block  ------------
/*
void
PlaygroundEDProducer::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when ending the processing of a luminosity block  ------------
/*
void
PlaygroundEDProducer::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void PlaygroundEDProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.add<std::string>("folder", "HGCAL/RecHits");
  desc.add<std::string>("DataType", "beam");
  desc.add<std::vector<int>>("CalibrationFlags", {1, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  desc.add<std::string>("CalibrationCSVFile", "./meta_conditions/calibration_parameters.csv");
  descriptions.add("playgroundedproducer", desc);

  //---------- Definitions of calibration flags ----------#
  // calibration_flags[0]: pedestal subtraction
  // calibration_flags[1]: cm subtraction
  // calibration_flags[2]: BX-1 correction
  // calibration_flags[3]: gain linearization
  // calibration_flags[4]: charge collection efficiency
  // calibration_flags[5]: MIP scale
  // calibration_flags[6]: EM scale
  // calibration_flags[7]: zero suppression
  // calibration_flags[8]: hit energy calibration
  // calibration_flags[9]: ToA conversion
  //------------------------------------------------------#
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(PlaygroundEDProducer);
