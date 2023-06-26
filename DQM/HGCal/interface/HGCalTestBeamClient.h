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
#include "DataFormats/HGCalDigi/interface/HGCalElectronicsId.h"

#include <TBranch.h>
#include <TChain.h>
#include <TFile.h>
#include <TString.h>
#include <TTree.h>

#include "DQM/HGCal/interface/hgcalhit.h" // define DetectorId
#include "DQM/HGCal/interface/RunningCollection.h"
#include "DQM/HGCal/interface/LoadCalibrationParameters.h"

#include <TCollection.h> // for TIter
#include <TGraph.h>
#include <TH2Poly.h>
#include <TKey.h>
#include <TObject.h>

class HGCalTestBeamClient : public DQMEDAnalyzer {
public:
  explicit HGCalTestBeamClient(const edm::ParameterSet&);
  ~HGCalTestBeamClient() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void bookHistograms(DQMStore::IBooker&, edm::Run const&, edm::EventSetup const&) override;

  void analyze(const edm::Event&, const edm::EventSetup&) override;

  virtual void     Init(TTree *tree=0); // in order to load ntuple (temporary input from 2022 testbeam data)
  virtual Long64_t LoadTree(Long64_t entry);

  virtual void     enable_pedestal_subtraction();
  virtual void     enable_cm_subtraction();

  virtual void     fill_histograms();
  virtual void     fill_profiles(int globalChannelId_, double adc_double_, double adcm_double_);

  virtual void     export_calibration_parameters();

  // ------------ member data ------------
  std::string folder_;
  TString myTag;
  std::vector<int> calibration_flags;
  TString root_beamRun;
  TString root_pedestal;

  TString tag_calibration;
  TString tag_channelId;

  CalibrationParameterLoader calib_loader;

  RunningCollection myRunStatCollection;
  RunningStatistics myRecorder;

  bool flag_perform_pedestal_subtraction;
  bool flag_perform_cm_subtraction;

  int globalChannelId;
  double adc_double;
  double adcm_double;
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
  MonitorElement* p_adc_diff ;
  MonitorElement* p_tot      ;
  MonitorElement* p_toa      ;
  MonitorElement* p_trigtime ;

  // summary for running statistics
  MonitorElement* p_correlation ;
  MonitorElement* p_slope       ;
  MonitorElement* p_intercept   ;

  //--------------------------------------------------
  // for hexagonal histograms (developing)
  //--------------------------------------------------
  int hex_counter;
  //TH2Poly *hexagonal_histogram;
  MonitorElement* hex_channelId;
  MonitorElement* hex_pedestal;
  MonitorElement* hex_adc_minus_adcm;
  MonitorElement* hex_tot_mean;
  MonitorElement* hex_beam_center;

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

HGCalTestBeamClient::HGCalTestBeamClient(const edm::ParameterSet& iConfig)
    : folder_(iConfig.getParameter<std::string>("folder")),
    myTag(iConfig.getParameter<std::string>( "DataType" )),
    calibration_flags(iConfig.getParameter<std::vector<int> >( "CalibrationFlags" )),
    root_beamRun(iConfig.getParameter<std::string >( "TB2022beamData" )),
    root_pedestal(iConfig.getParameter<std::string >( "TB2022pedData" ))
{
    // load trees from beam data / pedestal run
    TString input = (myTag=="beam") ? root_beamRun : root_pedestal;
    printf("[INFO] Input rootfile: %s\n", input.Data());

    f1 = new TFile(input, "R");
    t1 = (TTree*) f1->Get("unpacker_data/hgcroc");
    Init(t1); // SetBranchAddress, init variables, etc.

    // determine which calibrations to perform
    if(calibration_flags[0]) enable_pedestal_subtraction();
    if(calibration_flags[1]) enable_cm_subtraction();

    calib_loader.loadParameters();
}

HGCalTestBeamClient::~HGCalTestBeamClient() {
    // TODO: is the destructor a proper place to export calibration parameters?
    export_calibration_parameters();
    printf("[INFO] This is the end of the job\n");
}

void HGCalTestBeamClient::bookHistograms(DQMStore::IBooker& ibook, edm::Run const& run, edm::EventSetup const& iSetup) {
    //--------------------------------------------------
    // Examples
    //--------------------------------------------------
    ibook.setCurrentFolder("HGCAL/Examples");
    example_ = ibook.book1D("EXAMPLE", "Example 1D", 20, 0., 10.);
    example2D_ = ibook.book2D("EXAMPLE_2D", "Example 2D", 20, 0, 20, 15, 0, 15);
    example3D_ = ibook.book3D("EXAMPLE_3D", "Example 3D", 20, 0, 20, 15, 0, 15, 25, 0, 25);
    exampleTProfile_ = ibook.bookProfile("EXAMPLE_TPROFILE", "Example TProfile", 20, 0, 20, 15, 0, 15);
    exampleTProfile2D_ = ibook.bookProfile2D("EXAMPLE_TPROFILE2D", "Example TProfile 2D", 20, 0, 20, 15, 0, 15, 0, 100);

    //--------------------------------------------------
    // summary of physical quantities
    //--------------------------------------------------
    ibook.setCurrentFolder("HGCAL/Summary");
    p_adc       = ibook.bookProfile("p_adc"      , ";channel;ADC"                 , 234 , 0 , 234 , 175 , -25 , 150 );
    p_adcm      = ibook.bookProfile("p_adcm"     , ";channel;ADC_{-1}"            , 234 , 0 , 234 , 550 , -50 , 500 );
    p_adc_diff  = ibook.bookProfile("p_adc_diff" , ";channel;ADC #minus ADC_{-1}" , 234 , 0 , 234 , 550 , -50 , 500 );
    p_tot       = ibook.bookProfile("p_tot"      , ";channel;ToT"                 , 234 , 0 , 234 , 100 , -2  , 2   );
    p_toa       = ibook.bookProfile("p_toa"      , ";channel;ToA"                 , 234 , 0 , 234 , 500 , 0   , 500 );
    p_trigtime  = ibook.bookProfile("p_trigtime" , ";channel;trigtime"            , 234 , 0 , 234 , 500 , 0   , 500 );

    p_correlation = ibook.bookProfile("p_correlation" , ";channel;Correlation" , 234 , -0.5 , 233.5, 10, 0, 1);
    p_slope       = ibook.bookProfile("p_slope"       , ";channel;Slope"       , 234 , -0.5 , 233.5, 100, -5, +5);
    p_intercept   = ibook.bookProfile("p_intercept"   , ";channel;Intercept"   , 234 , -0.5 , 233.5, 100, -5, +5);

    //--------------------------------------------------
    // distributions of a specific channel
    //--------------------------------------------------
    ibook.setCurrentFolder("HGCAL/Digis");
    h_adc       = ibook.book1D("h_adc"      + tag_channelId , ";ADC;Entries"      , 175 , -25 , 150 );
    h_adcm      = ibook.book1D("h_adcm"     + tag_channelId , ";ADC-1;Entries"    , 550 , -50 , 500 );
    h_tot       = ibook.book1D("h_tot"      + tag_channelId , ";ToT;Entries"      , 100 , -2  , 2   );
    h_toa       = ibook.book1D("h_toa"      + tag_channelId , ";ToA;Entries"      , 500 , 0   , 500 );
    h_trigtime  = ibook.book1D("h_trigtime" + tag_channelId , ";trigtime;Entries" , 500 , 0   , 500 );

    h2d_adc          = ibook.book2D      ("h2d_adc"          + tag_channelId , ";CM #minus CM_{pedestal};ADC #minus ADC_{pedestal}", 19, -9.5, 9.5, 39, -9.5, 29.5);
    p2d_adc          = ibook.bookProfile ("p2d_adc"          + tag_channelId , ";CM #minus CM_{pedestal};ADC #minus ADC_{pedestal}", 19, -9.5, 9.5, 39, -9.5, 29.5);
    h2d_adc_trigtime = ibook.book2D      ("h2d_adc_trigtime" + tag_channelId , ";Trigger time;ADC #minus ADC_{pedestal}"           , 50,   30,  80, 39, -9.5, 29.5);

    //--------------------------------------------------
    // load geometry
    //--------------------------------------------------
    TString root_geometry = "/afs/cern.ch/work/y/ykao/public/raw_data_handling/hexagons_20230626.root";
    TFile *fgeo = new TFile(root_geometry, "R");

    ibook.setCurrentFolder("HGCAL/Maps");
    TString xytitle = ";x (cm);y (cm)";
    hex_channelId      = ibook.book2DPoly("hex_channelId"      , "hex_channelId"      + xytitle , -26 , 26 , -28 , 24);
    hex_pedestal       = ibook.book2DPoly("hex_pedestal"       , "hex_pedestal"       + xytitle , -26 , 26 , -28 , 24);
    hex_adc_minus_adcm = ibook.book2DPoly("hex_adc_minus_adcm" , "hex_adc_minus_adcm" + xytitle , -26 , 26 , -28 , 24);
    hex_tot_mean       = ibook.book2DPoly("hex_tot_mean"       , "hex_tot_mean"       + xytitle , -26 , 26 , -28 , 24);
    hex_beam_center    = ibook.book2DPoly("hex_beam_center"    , "hex_beam_center"    + xytitle , -26 , 26 , -28 , 24);

    // CAVEAT: the current bin number represents padId, instead of channelId
    hex_counter = 0;
    TGraph *gr;
    TKey *key;
    TIter nextkey(fgeo->GetDirectory(nullptr)->GetListOfKeys());
    while ((key = (TKey*)nextkey())) {
        TObject *obj = key->ReadObj();
        if(obj->InheritsFrom("TGraph")) {
            gr = (TGraph*) obj;
            hex_channelId      -> addBin(gr);
            hex_pedestal       -> addBin(gr);
            hex_adc_minus_adcm -> addBin(gr);
            hex_tot_mean       -> addBin(gr);
            hex_beam_center    -> addBin(gr);
            hex_counter+=1;
        }
    }

    for(int i=0; i<hex_counter; ++i) {
        if(i==0)
            hex_channelId->setBinContent(i+1, 1e-6);
        else
            hex_channelId->setBinContent(i+1, i);
    }
    fgeo->Close();
}

// ------------ auxilliary methods  ------------
void HGCalTestBeamClient::export_calibration_parameters() {
    // (0, 0) -> produce a table of derived parameters
    // (1, 0) -> produce a table of applied pedestal & derived CM parameters
    // (1, 1) -> do not produce a table

    if(calibration_flags[0]==1 && calibration_flags[1]==1) {
        printf("[INFO] both pedestal subtraction and CM subtraction are applied. No need to export parameters.\n");
    } else {
        TString csv_file_name = "./meta_conditions/output_DQMEDAnalyzer_calibration_parameters_" + myTag + "Data" + tag_calibration + ".txt";
        std::ofstream myfile(csv_file_name.Data());
        myfile << "Channel Pedestal CM_slope CM_offset kappa_BXm1\n";

        std::vector<RunningStatistics> mRs = myRunStatCollection.get_vector_running_statistics();
        for(int channelId=0; channelId<234; ++channelId) {
            double kappa_BXm1 = 0.000;
            bool isCM = ( channelId%39==37 || channelId%39==38 );
            RunningStatistics rs = mRs[channelId];
            //HGCalElectronicsId id (isCM, 0, 0, 0, int(channelId/39), channelId%39);
            HGCalElectronicsId id (0, 0, 0, int(channelId/39), channelId%39);

            int eleId = id.raw();
            double pedestal = (calibration_flags[0]==1) ? calib_loader.map_pedestals[eleId] : rs.get_mean_adc();
            myfile << Form("%d %f %f %f %f\n", id.raw(), pedestal, rs.get_slope(), rs.get_intercept(), kappa_BXm1);
        }

        myfile.close();
        printf("[INFO] export CM parameters: %s\n", csv_file_name.Data());
    }
}

Long64_t HGCalTestBeamClient::LoadTree(Long64_t entry)
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

void HGCalTestBeamClient::Init(TTree *tree)
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
    tag_channelId = "_channel_22";
}

void HGCalTestBeamClient::enable_pedestal_subtraction() { flag_perform_pedestal_subtraction = true; tag_calibration = "_ped_subtracted"; }

void HGCalTestBeamClient::enable_cm_subtraction() { flag_perform_cm_subtraction = true; tag_calibration = "_cm_subtracted"; }

void HGCalTestBeamClient::fill_histograms()
{
    myRecorder.add_entry(adc_channel_CM, adc_double);

    h2d_adc          -> Fill(adc_channel_CM, adc_double);
    p2d_adc          -> Fill(adc_channel_CM, adc_double);
    h2d_adc_trigtime -> Fill(trigtime      , adc_double);

    h_adc      -> Fill(adc_double);
    h_adcm     -> Fill(adcm);
    h_tot      -> Fill(tot);
    h_toa      -> Fill(toa);
    h_trigtime -> Fill(trigtime);
}

void HGCalTestBeamClient::fill_profiles(int globalChannelId_, double adc_double_, double adcm_double_)
{
    myRunStatCollection.add_entry(globalChannelId_, adc_double_, adc_channel_CM);

    p_adc      -> Fill(globalChannelId_ , adc_double_              );
    p_adcm     -> Fill(globalChannelId_ , adcm_double_             );
    p_adc_diff -> Fill(globalChannelId_ , adc_double_-adcm_double_ );
    p_tot      -> Fill(globalChannelId_ , tot                      );
    p_toa      -> Fill(globalChannelId_ , toa                      );
    p_trigtime -> Fill(globalChannelId_ , trigtime                 );
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void HGCalTestBeamClient::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<std::string>("folder", "HGCAL/Digis");
    desc.add<std::string>("DataType", "beam");
    desc.add<std::vector<int>>("CalibrationFlags", {1, 1, 0, 0, 0, 0, 0, 0, 0, 0});
    desc.add<std::string>("TB2022beamData", "/eos/cms/store/group/dpg_hgcal/tb_hgcal/2022/sps_oct2022/pion_beam_150_320fC/beam_run/run_20221007_191926/beam_run0.root");
    desc.add<std::string>("TB2022pedData", "/eos/cms/store/group/dpg_hgcal/tb_hgcal/2022/sps_oct2022/pedestals/pedestal_320fC/pedestal_run/run_20221008_192720/pedestal_run0.root");
    descriptions.add("playgrounddqmedanalyzer", desc);

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
}
