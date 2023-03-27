#!/usr/bin/env python
import ROOT
ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat("nemrou");
ROOT.gStyle.SetPalette(ROOT.kCherry)
ROOT.TColor.InvertPalette()

import os
import datetime
directory='./eos/plots_{}/'.format(datetime.datetime.now().strftime('%Y%b%d'))
os.system('mkdir -p {}'.format(directory)) 
tag_channel = "_channel_22"

    
def extract_single_plot(d, c1, tag, hname):
    h = d.Get(hname)
    h.Draw()
    output = directory + hname + tag
    c1.SaveAs(output + ".png")

def make_h2d_plot(fname, tag):
    fin = ROOT.TFile(fname, "R")
    d = fin.GetDirectory("DQMData/Run 1/HGCAL/Run summary/RecHits", True)
    
    h2d = d.Get("h2d_adc" + tag_channel)
    p2d = d.Get("p2d_adc" + tag_channel)
    
    #--------------------------------------------------
    # Linear fit
    #--------------------------------------------------
    ''' Consider to add linear fit based on h2d from DQM plots '''
    
    #--------------------------------------------------
    # Running statistics
    #--------------------------------------------------
    ''' To add the information on the plot, we need to load values from a parameter table produced from DQM module '''
    
    #--------------------------------------------------
    # plotting
    #--------------------------------------------------
    c1 = ROOT.TCanvas("c1", "", 800, 600)
    c1.SetRightMargin(0.15)
    
    h2d.SetStats(0)
    h2d.Draw("colz")
    
    p2d.SetStats(0)
    p2d.SetMarkerStyle(20)
    p2d.SetMarkerSize(0.5)
    p2d.SetLineWidth(2)
    p2d.Draw("p,same")
    
    output = directory + "h2d_adc" + tag_channel + tag
    c1.SaveAs(output + ".png")
    c1.SaveAs(output + ".pdf")

    #--------------------------------------------------
    # plot trig time
    #--------------------------------------------------
    h2d_trig = d.Get("h2d_adc_trigtime" + tag_channel)
    h2d_trig.Draw("colz")
    output = directory + "h2d_adc_trigtime" + tag_channel + tag
    c1.SaveAs(output + ".png")
    c1.SaveAs(output + ".pdf")

    #--------------------------------------------------
    # summary plots
    #--------------------------------------------------
    extract_single_plot(d, c1, tag, "p_adc")
    extract_single_plot(d, c1, tag, "p_correlation")
    extract_single_plot(d, c1, tag, "p_slope")
    extract_single_plot(d, c1, tag, "p_intercept")

if __name__ == "__main__":
    make_h2d_plot("./eos/DQM_V0001_TEST_R000000001.root", "")
    #make_h2d_plot("./eos/DQM_V0001_CMTEST_R000000001.root", "_cm_subtracted")
