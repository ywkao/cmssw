//#include "CalibCalorimetry/PlaygroundDQMEDAnalyzer/interface/PlaygroundDQMEDAnalyzer.h"
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
#include "DataFormats/HGCalDigi/interface/HGCROCChannelDataFrame.h"
#include "DataFormats/HGCalDigi/interface/HGCalDigiCollections.h"

#include <TBranch.h>
#include <TChain.h>
#include <TFile.h>
#include <TString.h>
#include <TTree.h>

//#include "DQM/HGCal/interface/hgcalhit.h" // define DetectorId
#include "DQM/HGCal/interface/RunningCollection.h"
#include "DQM/HGCal/interface/LoadCalibrationParameters.h"

class PlaygroundDQMEDAnalyzer : public DQMEDAnalyzer {
public:
  explicit PlaygroundDQMEDAnalyzer(const edm::ParameterSet&);
  ~PlaygroundDQMEDAnalyzer() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void bookHistograms(DQMStore::IBooker&, edm::Run const&, edm::EventSetup const&) override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  const edm::EDGetTokenT<HGCalElecDigiCollection> elecDigisToken_;

  MonitorElement* p_adc_minus_adcm1;
  MonitorElement* hex_adc_minus_adcm1;
  MonitorElement* hex_tot_median;
  MonitorElement* hex_beam_center;

/*
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
*/
};


PlaygroundDQMEDAnalyzer::PlaygroundDQMEDAnalyzer(const edm::ParameterSet& iConfig)
    : elecDigisToken_(consumes<HGCalElecDigiCollection>(iConfig.getParameter<edm::InputTag>("Digis")))
{}

/*
PlaygroundDQMEDAnalyzer::PlaygroundDQMEDAnalyzer(const edm::ParameterSet& iConfig)
    : folder_(iConfig.getParameter<std::string>("folder")),
    myTag(iConfig.getParameter<std::string>( "DataType" )),
    calibration_flags(iConfig.getParameter<std::vector<int> >( "CalibrationFlags" ))
{

    // load trees from beam data / pedestal run
    TString root_beamRun  = "/eos/cms/store/group/dpg_hgcal/tb_hgcal/2022/sps_oct2022/pion_beam_150_320fC/beam_run/run_20221007_191926/beam_run0.root";
    TString root_pedestal = "/eos/cms/store/group/dpg_hgcal/tb_hgcal/2022/sps_oct2022/pedestals/pedestal_320fC/pedestal_run/run_20221008_192720/pedestal_run0.root";
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
*/

PlaygroundDQMEDAnalyzer::~PlaygroundDQMEDAnalyzer() {
    // TODO: is the destructor a proper place to export calibration parameters?
    // export_calibration_parameters();
    printf("[INFO] This is the end of the job\n");
}

// ------------ method called for each event  ------------
void PlaygroundDQMEDAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm;

    const auto& elecDigis = iEvent.get(elecDigisToken_);
    for (auto& elecDigi : elecDigis) {
        std::cout<<"Electronics Id="<<elecDigi.id().raw()<<std::endl;
        elecDigi.id().print();
        elecDigi.print();
        std::cout << "adc = " << elecDigi.adc() << ", "
                  << "adcm1 = " << elecDigi.adcm1() << ", "
                  << "tot = " << elecDigi.tot() << ", "
                  << "halfrocChannel = " << (uint32_t) elecDigi.id().halfrocChannel() << ", "
                  << "sequentialHalfrocChannel = " << (uint32_t) elecDigi.id().sequentialHalfrocChannel()
                  << std::endl;
        
        // Need logial mapping / electronics mapping
        uint16_t adc_diff = elecDigi.adc() - elecDigi.adcm1(); 
        p_adc_minus_adcm1->Fill( (uint32_t)elecDigi.id().halfrocChannel(), adc_diff);
    }

    /*
    eventCount_++;

    example_->Fill(5);
    example2D_->Fill(eventCount_ / 10, eventCount_ / 10);
    example3D_->Fill(eventCount_ / 10, eventCount_ / 10, eventCount_ / 10.f);
    exampleTProfile_->Fill(eventCount_ / 10, eventCount_ / 10.f);
    exampleTProfile2D_->Fill(eventCount_ / 10, eventCount_ / 10, eventCount_ / 10.f);

    //--------------------------------------------------
    // migrate from a standalone c++ code
    //--------------------------------------------------
    Long64_t nentries = fChain->GetEntriesFast();
    int nevent = nentries / 78;

    std::cout << ">>> nentries = " << nentries << std::endl;
    std::cout << ">>> nevent = " << nevent << std::endl;

    // loop over hit
    std::vector<Hit> hits[nevent];
    int current_half      = 0;
    int recorded_half     = 0;
    double adc_channel_37 = 0.;

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

        // need eleId for reading calibration parameters
        bool is_cm_channel = (globalChannelId % 39 == 37 || globalChannelId % 39 == 38);
        HGCalElectronicsId id (is_cm_channel, 0, 0, 0, int(globalChannelId/39), globalChannelId%39);
        int eleId = id.raw();

        // convert adc to double
        adc_double = (double) adc;

        // perform pedestal subtraction
        if(flag_perform_pedestal_subtraction) {
            double pedestal = calib_loader.map_pedestals[eleId];
            adc_double -= pedestal;
        }

        // handle cm information after pedestal subtraction
        if(is_cm_channel) {
            // take average of two cm channels in a half
            adc_channel_CM += adc_double / 2.;
        }

        // record adc of ch37 & fill info of ch37 when processing ch38
        if(globalChannelId % 39 == 37) {
            adc_channel_37 = adc_double;
            continue;

        } else if(globalChannelId % 39 == 38) {
            // CM subtraction for channel 37
            if(flag_perform_cm_subtraction) {
                std::vector<double> parameters = calib_loader.map_cm_parameters[eleId-1];
                double slope = parameters[0];
                double intercept = parameters[1];
                double correction = adc_channel_CM*slope + intercept;
                adc_channel_37 -= correction;
            }

            fill_profiles(globalChannelId-1, adc_channel_37);
        }

        // perform common mode subtraction
        if(flag_perform_cm_subtraction) {
            std::vector<double> parameters = calib_loader.map_cm_parameters[eleId];
            double slope = parameters[0];
            double intercept = parameters[1];
            double correction = adc_channel_CM*slope + intercept;
            adc_double -= correction;
        }

        fill_profiles(globalChannelId, adc_double);

        if(globalChannelId==22) fill_histograms();
    } // end of ntpule hit loop

    // summary for running statistics
    std::vector<RunningStatistics> mRs = myRunStatCollection.get_vector_running_statistics();
    for(int channelId=0; channelId<234; ++channelId) {
        // TODO: how to set uncertainty?
        p_correlation -> Fill( channelId+1, mRs[channelId].get_correlation() );
        p_slope       -> Fill( channelId+1, mRs[channelId].get_slope()       );
        p_intercept   -> Fill( channelId+1, mRs[channelId].get_intercept()   );
    }
    */
}

void PlaygroundDQMEDAnalyzer::bookHistograms(DQMStore::IBooker& ibook, edm::Run const& run, edm::EventSetup const& iSetup) {
    p_adc_minus_adcm1 = ibook.bookProfile("p_adc_minus_adcm1", ";channel;ADC #minux ADC_{-1}", 234 , 0 , 234 , 175 , -25 , 150 );

    // Need the DQM service implemented with TH2Poly
    //hex_adc_minus_adcm1 = ibook.book2DPoly("hex_adc_minus_adcm1", "hex_adc_minus_adcm1;x (arb. unit); y (arb. unit)", -32, 32, -32, 32);
    //hex_tot_median      = ibook.book2DPoly("hex_tot_median", "hex_tot_median;x (arb. unit); y (arb. unit)", -32, 32, -32, 32);
    //hex_beam_center     = ibook.book2DPoly("hex_beam_center", "hex_beam_center;x (arb. unit); y (arb. unit)", -32, 32, -32, 32);

    /*
    ibook.setCurrentFolder(folder_);

    example_ = ibook.book1D("EXAMPLE", "Example 1D", 20, 0., 10.);
    example2D_ = ibook.book2D("EXAMPLE_2D", "Example 2D", 20, 0, 20, 15, 0, 15);
    example3D_ = ibook.book3D("EXAMPLE_3D", "Example 3D", 20, 0, 20, 15, 0, 15, 25, 0, 25);
    exampleTProfile_ = ibook.bookProfile("EXAMPLE_TPROFILE", "Example TProfile", 20, 0, 20, 15, 0, 15);
    exampleTProfile2D_ = ibook.bookProfile2D("EXAMPLE_TPROFILE2D", "Example TProfile 2D", 20, 0, 20, 15, 0, 15, 0, 100);

    // an instance of distributions
    h_adc       = ibook.book1D("h_adc"      + tag_channelId , ";ADC;Entries"      , 175 , -25 , 150 );
    h_adcm      = ibook.book1D("h_adcm"     + tag_channelId , ";ADC-1;Entries"    , 550 , -50 , 500 );
    h_tot       = ibook.book1D("h_tot"      + tag_channelId , ";ToT;Entries"      , 100 , -2  , 2   );
    h_toa       = ibook.book1D("h_toa"      + tag_channelId , ";ToA;Entries"      , 500 , 0   , 500 );
    h_trigtime  = ibook.book1D("h_trigtime" + tag_channelId , ";trigtime;Entries" , 500 , 0   , 500 );

    h2d_adc          = ibook.book2D      ("h2d_adc"          + tag_channelId , ";CM #minus CM_{pedestal};ADC #minus ADC_{pedestal}", 19, -9.5, 9.5, 39, -9.5, 29.5);
    p2d_adc          = ibook.bookProfile ("p2d_adc"          + tag_channelId , ";CM #minus CM_{pedestal};ADC #minus ADC_{pedestal}", 19, -9.5, 9.5, 39, -9.5, 29.5);
    h2d_adc_trigtime = ibook.book2D      ("h2d_adc_trigtime" + tag_channelId , ";Trigger time;ADC #minus ADC_{pedestal}"           , 50,   30,  80, 39, -9.5, 29.5);

    // summary of physical quantities
    p_adc       = ibook.bookProfile("p_adc"      , ";channel;ADC"      , 234 , 0 , 234 , 175 , -25 , 150 );
    p_adcm      = ibook.bookProfile("p_adcm"     , ";channel;ADC-1"    , 234 , 0 , 234 , 550 , -50 , 500 );
    p_tot       = ibook.bookProfile("p_tot"      , ";channel;ToT"      , 234 , 0 , 234 , 100 , -2  , 2   );
    p_toa       = ibook.bookProfile("p_toa"      , ";channel;ToA"      , 234 , 0 , 234 , 500 , 0   , 500 );
    p_trigtime  = ibook.bookProfile("p_trigtime" , ";channel;trigtime" , 234 , 0 , 234 , 500 , 0   , 500 );
    p_status    = ibook.bookProfile("p_status"   , ";channel;status"   , 234 , 0 , 234 , 3   , -1  , 1   );

    // summary of running statistics
    p_correlation = ibook.bookProfile("p_correlation" , ";channel;Correlation" , 234 , -0.5 , 233.5, 10, 0, 1);
    p_slope       = ibook.bookProfile("p_slope"       , ";channel;Slope"       , 234 , -0.5 , 233.5, 100, -5, +5);
    p_intercept   = ibook.bookProfile("p_intercept"   , ";channel;Intercept"   , 234 , -0.5 , 233.5, 100, -5, +5);
    */
}

/*
// ------------ auxilliary methods  ------------
void PlaygroundDQMEDAnalyzer::export_calibration_parameters() {
    TString csv_file_name = "./meta_conditions/output_DQMEDAnalyzer_calibration_parameters_" + myTag + "Data" + tag_calibration + ".txt";
    std::ofstream myfile(csv_file_name.Data());

    std::vector<RunningStatistics> mRs = myRunStatCollection.get_vector_running_statistics();

    myfile << "Channel Pedestal CM_slope CM_offset kappa_BXm1\n";
    for(int channelId=0; channelId<234; ++channelId) {
        double kappa_BXm1 = 0.000;
        bool isCM = ( channelId%39==37 || channelId%39==38 );
        RunningStatistics rs = mRs[channelId];
        HGCalElectronicsId id (isCM, 0, 0, 0, int(channelId/39), channelId%39);
        myfile << Form("%d %f %f %f %f\n", id.raw(), rs.get_mean_adc(), rs.get_slope(), rs.get_intercept(), kappa_BXm1);
    }
    myfile.close();
    printf("[INFO] export CM parameters: %s\n", csv_file_name.Data());
}

Long64_t PlaygroundDQMEDAnalyzer::LoadTree(Long64_t entry)
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

void PlaygroundDQMEDAnalyzer::Init(TTree *tree)
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

void PlaygroundDQMEDAnalyzer::enable_pedestal_subtraction() { flag_perform_pedestal_subtraction = true; tag_calibration = "_ped_subtracted"; }

void PlaygroundDQMEDAnalyzer::enable_cm_subtraction() { flag_perform_cm_subtraction = true; tag_calibration = "_cm_subtracted"; }

void PlaygroundDQMEDAnalyzer::fill_histograms()
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

void PlaygroundDQMEDAnalyzer::fill_profiles(int globalChannelId_, double adc_double_)
{
    myRunStatCollection.add_entry(globalChannelId_, adc_double_, adc_channel_CM);

    p_adc      -> Fill(globalChannelId_ , adc_double_);
    p_adcm     -> Fill(globalChannelId_ , adcm      );
    p_tot      -> Fill(globalChannelId_ , tot       );
    p_toa      -> Fill(globalChannelId_ , toa       );
    p_trigtime -> Fill(globalChannelId_ , trigtime  );
}
*/

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void PlaygroundDQMEDAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("Digis",edm::InputTag("hgcalDigis","DIGI","TEST"));
}

// define this as a plug-in
DEFINE_FWK_MODULE(PlaygroundDQMEDAnalyzer);
