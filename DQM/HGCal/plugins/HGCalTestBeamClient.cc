#include "DQM/HGCal/interface/HGCalTestBeamClient.h"

// ------------ method called for each event  ------------
void HGCalTestBeamClient::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
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

    std::vector<Hit> hits[nevent];
    int current_half      = 0;
    int recorded_half     = 0;
    double adc_channel_37 = 0.;
    double adcm_channel_37 = 0.;

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
        //HGCalElectronicsId id (is_cm_channel, 0, 0, 0, int(globalChannelId/39), globalChannelId%39);
        HGCalElectronicsId id (0, 0, 0, int(globalChannelId/39), globalChannelId%39);
        int eleId = id.raw();
        // convert adc to double
        adc_double = (double) adc;
        adcm_double = (double) adcm;

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
            adcm_channel_37 = adc_double;
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

            fill_profiles(globalChannelId-1, adc_channel_37, adcm_channel_37);
        }

        // perform common mode subtraction
        if(flag_perform_cm_subtraction) {
            std::vector<double> parameters = calib_loader.map_cm_parameters[eleId];
            double slope = parameters[0];
            double intercept = parameters[1];
            double correction = adc_channel_CM*slope + intercept;
            adc_double -= correction;
        }

        fill_profiles(globalChannelId, adc_double, adcm_double);

        if(globalChannelId==22) fill_histograms();
    } // end of ntpule hit loop

    //--------------------------------------------------
    // summary for running statistics
    //--------------------------------------------------
    int max_bin_diff_adc = -1;
    double max_diff_adc = -1.;
    std::vector<RunningStatistics> mRs = myRunStatCollection.get_vector_running_statistics();
    for(int channelId=0; channelId<234; ++channelId) {
        bool is_cm_channel = (channelId % 39 == 37 || channelId % 39 == 38);

        // problems with setBinContent(): 1. the plots look empty 2. the entries not as expected
        p_correlation -> setBinContent( channelId+1, mRs[channelId].get_correlation() );
        p_slope       -> setBinContent( channelId+1, mRs[channelId].get_slope()       );
        p_intercept   -> setBinContent( channelId+1, mRs[channelId].get_intercept()   );

        if(channelId<hex_counter) {
            double mean_adc_diff = p_adc_diff->getBinContent(channelId+1);
            double mean_tot = p_tot->getBinContent(channelId+1);

            hex_pedestal->setBinContent(channelId+1, mRs[channelId].get_mean_adc());
            hex_adc_minus_adcm -> setBinContent(channelId+1, mean_adc_diff);
            hex_tot_mean       -> setBinContent(channelId+1, mean_tot);

            if(!is_cm_channel && mean_adc_diff>max_diff_adc) {
                max_bin_diff_adc = channelId;
                max_diff_adc = mean_adc_diff;
            }
        }
    }
    hex_beam_center -> setBinContent(max_bin_diff_adc+1, max_diff_adc);

} // end of analyze

// define this as a plug-in
DEFINE_FWK_MODULE(HGCalTestBeamClient);
