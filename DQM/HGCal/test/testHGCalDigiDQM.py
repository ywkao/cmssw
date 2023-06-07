import FWCore.ParameterSet.Config as cms
import FWCore.ParameterSet.VarParsing as VarParsing

process = cms.Process("testHGCalDQMdigis")

process.source = cms.Source("PoolSource",
   fileNames = cms.untracked.vstring('file:/afs/cern.ch/user/y/yumiao/public/HGCAL_Raw_Data_Handling/Data/Digis/testFakeDigis.root')
)

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1) )

process.playgrounddqmedanalyzer = cms.EDProducer('PlaygroundDQMEDAnalyzer',
    Digis = cms.InputTag('hgcalDigis', 'DIGI', 'TEST'),
)

process.DQMStore = cms.Service("DQMStore")

process.load("DQMServices.FileIO.DQMFileSaverOnline_cfi")
process.dqmSaver.tag = 'HGCAL'
process.dqmSaver.path = './eos/'
process.dqmSaver.runNumber = 123466

process.p = cms.Path(process.playgrounddqmedanalyzer + process.dqmSaver)

# process.output = cms.OutputModule("PoolOutputModule",
#     fileName = cms.untracked.string("./testRecHit.root"),
# )
# 
# process.outpath = cms.EndPath(process.output)
# 
# process.hgcalRecHits = cms.EDProducer('HGCalDigiToRecHit',
#   Digis = cms.InputTag('hgcalDigis', '', 'TEST'),
# )

