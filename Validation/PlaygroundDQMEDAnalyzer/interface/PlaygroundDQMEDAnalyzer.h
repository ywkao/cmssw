#include <string>
#include <fstream>
#include <iostream>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DQMServices/Core/interface/DQMEDAnalyzer.h"
#include "DQMServices/Core/interface/MonitorElement.h"

#include <TBranch.h>
#include <TChain.h>
#include <TFile.h>
#include <TString.h>
#include <TTree.h>

#include "Validation/PlaygroundDQMEDAnalyzer/interface/hgcalhit.h" // define DetectorId
#include "Validation/PlaygroundDQMEDAnalyzer/interface/RunningCollection.h"
#include "Validation/PlaygroundDQMEDAnalyzer/interface/LoadCalibrationParameters.h"

#include <TCollection.h> // for TIter
#include <TGraph.h>
#include <TH2Poly.h>
#include <TKey.h>
#include <TObject.h>

class PlaygroundDQMEDAnalyzer : public DQMEDAnalyzer {
public:
  explicit PlaygroundDQMEDAnalyzer(const edm::ParameterSet&);
  ~PlaygroundDQMEDAnalyzer() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void bookHistograms(DQMStore::IBooker&, edm::Run const&, edm::EventSetup const&) override;

  void analyze(const edm::Event&, const edm::EventSetup&) override;

  virtual void     Init(TTree *tree=0); // in order to load ntuple (temporary input from 2022 testbeam data)
  virtual Long64_t LoadTree(Long64_t entry);

  virtual void     enable_pedestal_subtraction();
  virtual void     enable_cm_subtraction();

  virtual void     fill_histograms();
  virtual void     fill_profiles(int globalChannelId_, double adc_double_);

  virtual void     export_calibration_parameters();

  // ------------ member data ------------
  std::string folder_;
  TString myTag;
  std::vector<int> calibration_flags;

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

  MonitorElement* example_;
  MonitorElement* example2D_;
  MonitorElement* example3D_;
  MonitorElement* exampleTProfile_;
  MonitorElement* exampleTProfile2D_;
  int eventCount_ = 0;

  // an instance of distributions
  MonitorElement* h_adc      ;
  MonitorElement* h_adcm     ;
  MonitorElement* h_tot      ;
  MonitorElement* h_toa      ;
  MonitorElement* h_trigtime ;

  MonitorElement* h2d_adc    ;
  MonitorElement* p2d_adc    ;
  MonitorElement* h2d_adc_trigtime    ;

  // summary of physical quantities
  MonitorElement* p_adc      ;
  MonitorElement* p_adcm     ;
  MonitorElement* p_tot      ;
  MonitorElement* p_toa      ;
  MonitorElement* p_trigtime ;
  MonitorElement* p_status   ;

  // summary for running statistics
  MonitorElement* p_correlation ;
  MonitorElement* p_slope       ;
  MonitorElement* p_intercept   ;

  //--------------------------------------------------
  // for hexagonal histograms (temporary)
  //--------------------------------------------------
  TH2Poly *hexagonal_histogram;

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

