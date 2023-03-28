#include <string>
#include <fstream>
#include <iostream>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DQMServices/Core/interface/DQMEDAnalyzer.h"
#include "DQMServices/Core/interface/MonitorElement.h"

#include <TBranch.h>
#include <TChain.h>
#include <TFile.h>
#include <TString.h>
#include <TTree.h>

#include "Validation/PlaygroundEDProducer/interface/hgcalhit.h" // data format
#include "Validation/PlaygroundEDProducer/interface/RunningCollection.h"

class testDQMEDAnalyzer : public DQMEDAnalyzer {
public:
  explicit testDQMEDAnalyzer(const edm::ParameterSet&);
  ~testDQMEDAnalyzer() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void bookHistograms(DQMStore::IBooker&, edm::Run const&, edm::EventSetup const&) override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  virtual void     enable_pedestal_subtraction() { tag_calibration = "_ped_subtracted"; }
  virtual void     enable_cm_subtraction()       { tag_calibration = "_cm_subtracted" ; }

  virtual void     export_calibration_parameters();

  // ------------ member data ------------
  edm::InputTag source_;
  edm::EDGetTokenT<RecHit> token_;
  std::string folder_;
  TString myTag;

  TString tag_calibration;
  TString tag_channelId;

  RunningCollection myRunStatCollection;
  RunningStatistics myRecorder;

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

  // summary of physical quantities
  MonitorElement* p_adc      ;
  MonitorElement* p_adcm     ;
  MonitorElement* p_tot      ;
  MonitorElement* p_toa      ;
  MonitorElement* p_trigtime ;
  MonitorElement* p_status   ;

  // summary for running statistics
  MonitorElement* h_correlation ;
  MonitorElement* h_slope       ;
  MonitorElement* h_intercept   ;
};

testDQMEDAnalyzer::testDQMEDAnalyzer(const edm::ParameterSet& iConfig) :
    source_{iConfig.getParameter<edm::InputTag>("source")},
    token_{consumes(source_)},
    folder_(iConfig.getParameter<std::string>("folder")),
    myTag(iConfig.getParameter<std::string>( "DataType" )) {}

testDQMEDAnalyzer::~testDQMEDAnalyzer() {
    // TODO: is the destructor a proper place to export calibration parameters?
    export_calibration_parameters();
    printf("[INFO] This is the end of the job\n");
}

// ------------ method called for each event  ------------
void testDQMEDAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm;

    RecHit hit = iEvent.get(token_);

    int globalChannelId  = hit.detid().id(); // chip*78+half*39+channel;
    float adc            = hit.adc();
    float adc_CM_channel = hit.adc_cm();
    float adcm           = hit.adcm();
    float tot            = hit.tot();
    float toa            = hit.toa();
    float trigtime       = hit.triglatency();

    // fill profiles
    myRunStatCollection.add_entry(globalChannelId, adc, adc_CM_channel);
    p_adc      -> Fill(globalChannelId , adc      );
    p_adcm     -> Fill(globalChannelId , adcm     );
    p_tot      -> Fill(globalChannelId , tot      );
    p_toa      -> Fill(globalChannelId , toa      );
    p_trigtime -> Fill(globalChannelId , trigtime );

    // fill a specific channel
    if(globalChannelId==7) {
        myRecorder.add_entry(adc_CM_channel, adc);
        h_adc      -> Fill(adc);
        h_adcm     -> Fill(adcm);
        h_tot      -> Fill(tot);
        h_toa      -> Fill(toa);
        h_trigtime -> Fill(trigtime);
    }

    // example code
    eventCount_++;
    example_->Fill(5);
    example2D_->Fill(eventCount_ / 10, eventCount_ / 10);
    example3D_->Fill(eventCount_ / 10, eventCount_ / 10, eventCount_ / 10.f);
    exampleTProfile_->Fill(eventCount_ / 10, eventCount_ / 10.f);
    exampleTProfile2D_->Fill(eventCount_ / 10, eventCount_ / 10, eventCount_ / 10.f);
}

void testDQMEDAnalyzer::bookHistograms(DQMStore::IBooker& ibook, edm::Run const& run, edm::EventSetup const& iSetup) {
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

    // summary of physical quantities
    p_adc       = ibook.bookProfile("p_adc"      , ";channel;ADC"      , 234 , 0 , 234 , 175 , -25 , 150 );
    p_adcm      = ibook.bookProfile("p_adcm"     , ";channel;ADC-1"    , 234 , 0 , 234 , 550 , -50 , 500 );
    p_tot       = ibook.bookProfile("p_tot"      , ";channel;ToT"      , 234 , 0 , 234 , 100 , -2  , 2   );
    p_toa       = ibook.bookProfile("p_toa"      , ";channel;ToA"      , 234 , 0 , 234 , 500 , 0   , 500 );
    p_trigtime  = ibook.bookProfile("p_trigtime" , ";channel;trigtime" , 234 , 0 , 234 , 500 , 0   , 500 );
    p_status    = ibook.bookProfile("p_status"   , ";channel;status"   , 234 , 0 , 234 , 3   , -1  , 1   );

    // summary of running statistics
    h_correlation = ibook.book1D("h_correlation" , ";channel;Correlation" , 234 , -0.5 , 233.5);
    h_slope       = ibook.book1D("h_slope"       , ";channel;Slope"       , 234 , -0.5 , 233.5);
    h_intercept   = ibook.book1D("h_intercept"   , ";channel;Intercept"   , 234 , -0.5 , 233.5);
}

// ------------ auxilliary methods  ------------
void testDQMEDAnalyzer::export_calibration_parameters() {
    TString csv_file_name = "./meta_conditions/output_DQMEDAnalyzer_calibration_parameters" + tag_calibration + ".csv";
    std::ofstream myfile(csv_file_name.Data());
    myfile << "#------------------------------------------------------------\n";
    myfile << "# info: " << myTag.Data() << "\n";
    myfile << "# columns: channel, pedestal, slope, intercept, correlation\n";
    myfile << "#------------------------------------------------------------\n";

    std::vector<RunningStatistics> mRs = myRunStatCollection.get_vector_running_statistics();

    for(int i=0; i<234; ++i) {
        myfile << Form("%d,%.2f,%.2f,%.2f,%.2f\n", i, mRs[i].get_mean_adc(), mRs[i].get_slope(), mRs[i].get_intercept(), mRs[i].get_correlation());
    }

    myfile.close();
    printf("[INFO] export CM parameters: %s\n", csv_file_name.Data());
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void testDQMEDAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("source"); // not sure how to wirte it correctly
    desc.add<std::string>("folder", "HGCAL/RecHits");
    desc.add<std::string>("DataType", "beam");
    descriptions.add("testdqmedanalyzer", desc);
}

// define this as a plug-in
DEFINE_FWK_MODULE(testDQMEDAnalyzer);
