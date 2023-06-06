#include "Validation/PlaygroundDQMEDAnalyzer/interface/PlaygroundDQMEDAnalyzer.h"

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

PlaygroundDQMEDAnalyzer::~PlaygroundDQMEDAnalyzer() {
    // TODO: is the destructor a proper place to export calibration parameters?
    export_calibration_parameters();
    printf("[INFO] This is the end of the job\n");
}

// ------------ method called for each event  ------------
void PlaygroundDQMEDAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm;

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
            continue;

        } else if(globalChannelId % 39 == 38) {
            // CM subtraction for channel 37
            if(flag_perform_cm_subtraction) {
                std::vector<double> parameters = calib_loader.map_cm_parameters[globalChannelId-1];
                double slope = parameters[0];
                double intercept = parameters[1];
                double correction = adc_channel_CM*slope + intercept;
                adc_channel_37 -= correction;
            }

            fill_profiles(globalChannelId-1, adc_channel_37);
        }

        // perform common mode subtraction
        if(flag_perform_cm_subtraction) {
            std::vector<double> parameters = calib_loader.map_cm_parameters[globalChannelId];
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
        // problems with setBinContent(): 1. the plots look empty 2. the entries not as expected
        p_correlation -> setBinContent( channelId+1, mRs[channelId].get_correlation() );
        p_slope       -> setBinContent( channelId+1, mRs[channelId].get_slope()       );
        p_intercept   -> setBinContent( channelId+1, mRs[channelId].get_intercept()   );
        printf("[DEBUG] channel %3d, corr = %.2f, slope = %.2f, intercept = %.2f\n",
                channelId,
                mRs[channelId].get_correlation(),
                mRs[channelId].get_slope(),
                mRs[channelId].get_intercept()
              );

        if(channelId<hex_counter)
            hex_pedestal->setBinContent(channelId+1, mRs[channelId].get_mean_adc());
            
        //// TODO: how to set uncertainty?
        //p_correlation -> Fill( channelId+1, mRs[channelId].get_correlation() );
        //p_slope       -> Fill( channelId+1, mRs[channelId].get_slope()       );
        //p_intercept   -> Fill( channelId+1, mRs[channelId].get_intercept()   );
    }
}

void PlaygroundDQMEDAnalyzer::bookHistograms(DQMStore::IBooker& ibook, edm::Run const& run, edm::EventSetup const& iSetup) {
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
    p_adc       = ibook.bookProfile("p_adc"      , ";channel;ADC"      , 234 , 0 , 234 , 175 , -25 , 150 );
    p_adcm      = ibook.bookProfile("p_adcm"     , ";channel;ADC-1"    , 234 , 0 , 234 , 550 , -50 , 500 );
    p_tot       = ibook.bookProfile("p_tot"      , ";channel;ToT"      , 234 , 0 , 234 , 100 , -2  , 2   );
    p_toa       = ibook.bookProfile("p_toa"      , ";channel;ToA"      , 234 , 0 , 234 , 500 , 0   , 500 );
    p_trigtime  = ibook.bookProfile("p_trigtime" , ";channel;trigtime" , 234 , 0 , 234 , 500 , 0   , 500 );
    p_status    = ibook.bookProfile("p_status"   , ";channel;status"   , 234 , 0 , 234 , 3   , -1  , 1   );

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
    TString root_geometry = "/afs/cern.ch/work/y/ykao/public/raw_data_handling/hexagon_20230606.root";
    TFile *fgeo = new TFile(root_geometry, "R");

    ibook.setCurrentFolder("HGCAL/Maps");
    hex_channelId = ibook.book2DPoly("hex_channelId", "hex_channelId;x (cm); y (cm)", -32, 32, -32, 32);
    hex_pedestal  = ibook.book2DPoly("hex_pedestal" , "hex_pedestal;x (cm); y (cm)", -32, 32, -32, 32);

    hex_counter = 0;
    TGraph *gr;
    TKey *key;
    TIter nextkey(fgeo->GetDirectory(nullptr)->GetListOfKeys());
    while ((key = (TKey*)nextkey())) {
        TObject *obj = key->ReadObj();
        if(obj->InheritsFrom("TGraph")) {
            gr = (TGraph*) obj;
            hex_channelId->addBin(gr);
            hex_pedestal->addBin(gr);
            hex_counter+=1;
        }
    }

    for(int i=0; i<hex_counter; ++i) hex_channelId->setBinContent(i+1, i+1);
    fgeo->Close();
}

// ------------ auxilliary methods  ------------
void PlaygroundDQMEDAnalyzer::export_calibration_parameters() {
    TString csv_file_name = "./meta_conditions/output_DQMEDAnalyzer_calibration_parameters" + tag_calibration + ".csv";
    std::ofstream myfile(csv_file_name.Data());
    myfile << "#------------------------------------------------------------\n";
    myfile << "# info: " << myTag.Data() << "\n";
    myfile << "# columns: channel, pedestal, slope, intercept, correlation\n";
    myfile << "#------------------------------------------------------------\n";

    std::vector<RunningStatistics> mRs = myRunStatCollection.get_vector_running_statistics();

    for(int i=0; i<234; ++i) {
        myfile << Form("%d,%.2f,%.2f,%.2f,%.2f\n", i, mRs[i].get_mean_adc(), mRs[i].get_slope(), mRs[i].get_intercept(), mRs[i].get_correlation());

        // the following method does not work because of L161 in DQMServices/Core/interface/MonitorElement.h
        if(i<hex_counter) {
            // double content = p_adc->getBinContent(i+1);
            // hex_pedestal->setBinContent(i+1, mRs[i].get_mean_adc());
        }
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
    myRunStatCollection.add_entry(globalChannelId_, adc_double, adc_channel_CM);

    p_adc      -> Fill(globalChannelId_ , adc_double);
    p_adcm     -> Fill(globalChannelId_ , adcm      );
    p_tot      -> Fill(globalChannelId_ , tot       );
    p_toa      -> Fill(globalChannelId_ , toa       );
    p_trigtime -> Fill(globalChannelId_ , trigtime  );
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void PlaygroundDQMEDAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<std::string>("folder", "HGCAL/Digis");
    desc.add<std::string>("DataType", "beam");
    desc.add<std::vector<int>>("CalibrationFlags", {1, 1, 0, 0, 0, 0, 0, 0, 0, 0});
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

// define this as a plug-in
DEFINE_FWK_MODULE(PlaygroundDQMEDAnalyzer);
