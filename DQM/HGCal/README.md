# PlaygroundDQMEDAnalyzer

Init DQM digis with input from fake digis data.

This branch is based on [hgcal-condformat-13_2_X](https://github.com/cms-hgcal/cmssw/tree/hgcal-condformat-13_2_X) with a few modifications in DataFormats/HGCalDigi.

```
# Developing on top of hgcal-condformat-13_2_X branch
cmsrel CMSSW_13_2_0_pre1
cd CMSSW_13_2_0_pre1/src/
cmsenv
git cms-init
git cms-checkout-topic CMS-HGCAL:hgcal-condformat-13_2_X
mkdir -p DQM/HGCal
cd DQM/HGCal

# Import this module
git clone -b dqmDigi git@github.com:ywkao/PlaygroundDQMEDAnalyzer.git
cd PlaygroundEDProducer/
mkdir eos
time scram b -j10
time cmsRun test/testHGCalDigiDQM.py
```

Reference: https://cms-sw.github.io/tutorial-collaborating-with-peers.html
