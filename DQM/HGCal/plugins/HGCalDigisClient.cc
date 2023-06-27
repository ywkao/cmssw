#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DQMServices/Core/interface/DQMEDAnalyzer.h"
#include "DQMServices/Core/interface/MonitorElement.h"

#include "DataFormats/HGCalDigi/interface/HGCalElectronicsId.h"
#include "DataFormats/HGCalDigi/interface/HGCROCChannelDataFrame.h"
#include "DataFormats/HGCalDigi/interface/HGCalDigiCollections.h"

#include <TString.h>
#include <TFile.h>
#include <TKey.h>

#include <string>
#include <fstream>
#include <iostream>

//#include "DQM/HGCal/interface/hgcalhit.h" // define DetectorId
#include "DQM/HGCal/interface/RunningCollection.h"
#include "DQM/HGCal/interface/LoadCalibrationParameters.h"

class HGCalDigisClient : public DQMEDAnalyzer {
public:
  explicit HGCalDigisClient(const edm::ParameterSet&);
  ~HGCalDigisClient() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void bookHistograms(DQMStore::IBooker&, edm::Run const&, edm::EventSetup const&) override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  const edm::EDGetTokenT<HGCalElecDigiCollection> elecDigisToken_;
  const edm::EDGetTokenT<std::vector<int> > metadataToken_;

  MonitorElement* p_adc      ;
  MonitorElement* p_adcm     ;
  MonitorElement* p_adc_minus_adcm1;
  MonitorElement* hex_channelId;
  MonitorElement* hex_adc_minus_adcm;
  MonitorElement* hex_tot_mean;
  MonitorElement* hex_beam_center;

  int hex_counter;

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
};


HGCalDigisClient::HGCalDigisClient(const edm::ParameterSet& iConfig)
  : elecDigisToken_(consumes<HGCalElecDigiCollection>(iConfig.getParameter<edm::InputTag>("Digis"))),
    metadataToken_(consumes<std::vector<int> >(iConfig.getParameter<edm::InputTag>("MetaData")))
{}

HGCalDigisClient::~HGCalDigisClient() {
    // TODO: is the destructor a proper place to export calibration parameters?
    // export_calibration_parameters();
  LogDebug("HGCalDigisClient") << "End of the job" << std::endl;
}

// ------------ method called for each event  ------------
void HGCalDigisClient::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm;

    //read trigtime
    const auto& metadata = iEvent.get(metadataToken_);
    int trigtime(-999);
    if(metadata.size()>0) trigtime=metadata.at(0);
    LogDebug("HGCalDigisClient") << "trigtime=" << trigtime;
    
    //read digis
    const auto& elecDigis = iEvent.get(elecDigisToken_);
    for (auto& elecDigi : elecDigis) {
      
      bool isCM=elecDigi.id().isCM();
      int ch=elecDigi.id().rocChannel();
      
      LogDebug("HGCalDigisClient") << "Electronics Id= 0x" << std::hex << elecDigi.id().raw() << std::dec << std::endl
                                   << "adc = " << elecDigi.adc() << ", "
                                   << "adcm1 = " << elecDigi.adcm1() << ", "
                                   << "tot = " << elecDigi.tot() << ", "
                                   << "halfrocChannel = " << (uint32_t) elecDigi.id().halfrocChannel() << ", "
                                   << "channel = " << ch << ", CM=" << isCM;
      
      // Note: need logial mapping / electronics mapping
      HGCalElectronicsId id = elecDigi.id();
      uint16_t adc      = elecDigi.adc(); 
      uint16_t adcm     = elecDigi.adcm1(); 
      uint16_t adc_diff = elecDigi.adc() - elecDigi.adcm1();
      int globalChannelId = 39*id.econdeRx() + id.halfrocChannel();
      p_adc             -> Fill( globalChannelId, adc      );
      p_adcm            -> Fill( globalChannelId, adcm     );
      p_adc_minus_adcm1 -> Fill( globalChannelId, adc_diff );

      //printf("[INFO] eleId = %d, econdeRx() = %d, halfrocChannel() = %d, chId = %d, adc = %d, adcm = %d, adc_diff = %d\n", id.raw(), id.econdeRx(), id.halfrocChannel(), globalChannelId, adc, adcm, adc_diff);
      // std::cout << "eleId: " << id.raw() << ","
      //           << "chId: " << globalChannelId << "," 
      //           << "adc: " << adc << ","
      //           << "adc_diff: " << adc_diff << std::endl;
      // // Note: need CM info
      // //myRunStatCollection.add_entry(globalChannelId_, adc_double_, adc_channel_CM);
    }
}

void HGCalDigisClient::bookHistograms(DQMStore::IBooker& ibook, edm::Run const& run, edm::EventSetup const& iSetup) {
    ibook.setCurrentFolder("HGCAL/Digis");
    p_adc             = ibook.bookProfile("p_adc"             , ";channel;ADC #minux ADC_{-1}" , 234 , 0 , 234 , 175 , -25 , 150 );
    p_adcm            = ibook.bookProfile("p_adcm"            , ";channel;ADC #minux ADC_{-1}" , 234 , 0 , 234 , 175 , -25 , 150 );
    p_adc_minus_adcm1 = ibook.bookProfile("p_adc_minus_adcm1" , ";channel;ADC #minux ADC_{-1}" , 234 , 0 , 234 , 175 , -25 , 150 );

    ibook.setCurrentFolder("HGCAL/Maps");
    TString xytitle = ";x (cm);y (cm)";
    hex_channelId      = ibook.book2DPoly("hex_channelId"      , "hex_channelId"      + xytitle , -26 , 26 , -28 , 24);
    hex_adc_minus_adcm = ibook.book2DPoly("hex_adc_minus_adcm" , "hex_adc_minus_adcm" + xytitle , -26 , 26 , -28 , 24);
    hex_tot_mean       = ibook.book2DPoly("hex_tot_mean"       , "hex_tot_mean"       + xytitle , -26 , 26 , -28 , 24);
    hex_beam_center    = ibook.book2DPoly("hex_beam_center"    , "hex_beam_center"    + xytitle , -26 , 26 , -28 , 24);

    //--------------------------------------------------
    // load geometry
    //--------------------------------------------------
    TString root_geometry = "/afs/cern.ch/work/y/ykao/public/raw_data_handling/hexagons_20230626.root";
    TFile *fgeo = new TFile(root_geometry, "R");

    hex_counter = 0;
    TGraph *gr;
    TKey *key;
    TIter nextkey(fgeo->GetDirectory(nullptr)->GetListOfKeys());
    while ((key = (TKey*)nextkey())) {
        TObject *obj = key->ReadObj();
        if(obj->InheritsFrom("TGraph")) {
            gr = (TGraph*) obj;
            hex_channelId      -> addBin(gr);
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

void HGCalDigisClient::export_calibration_parameters() {
    //TString csv_file_name = "./meta_conditions/output_DQMEDAnalyzer_calibration_parameters_" + myTag + "Data" + tag_calibration + ".txt";
    TString csv_file_name = "./meta_conditions/output_DQMEDAnalyzer_calibration_parameters.txt";
    std::ofstream myfile(csv_file_name.Data());

    std::vector<RunningStatistics> mRs = myRunStatCollection.get_vector_running_statistics();

    myfile << "Channel Pedestal CM_slope CM_offset kappa_BXm1\n";
    for(int channelId=0; channelId<234; ++channelId) {
        double kappa_BXm1 = 0.000;
        RunningStatistics rs = mRs[channelId];
        // Note: need CM info
        // bool isCM = ( channelId%39==37 || channelId%39==38 );
        HGCalElectronicsId id (1, 0, 0, 0, int(channelId/39), channelId%39);
        //HGCalElectronicsId id (0, 0, 0, int(channelId/39), channelId%39);
        myfile << Form("%d %f %f %f %f\n", id.raw(), rs.get_mean_adc(), rs.get_slope(), rs.get_intercept(), kappa_BXm1);
    }
    myfile.close();
    LogDebug("HGCalDigisClient") << "Export CM parameters @ " << csv_file_name << std::endl;
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void HGCalDigisClient::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("Digis",edm::InputTag("hgcalDigis"));
    desc.add<edm::InputTag>("MetaData",edm::InputTag("hgcalMetaData"));
}

// define this as a plug-in
DEFINE_FWK_MODULE(HGCalDigisClient);
