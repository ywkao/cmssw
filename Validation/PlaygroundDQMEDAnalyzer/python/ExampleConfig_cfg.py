import FWCore.ParameterSet.Config as cms

process = cms.Process("Demo")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents.input = 1
process.source = cms.Source('EmptySource')

#--------------------------------------------------
# ped subtraction
#--------------------------------------------------
#process.load("Validation.PlaygroundDQMEDAnalyzer.playgrounddqmedanalyzer_cfi")
process.playgrounddqmedanalyzer = cms.EDProducer('PlaygroundDQMEDAnalyzer',
  folder = cms.string('HGCAL/Digis'),
  DataType = cms.string('beam'),
  CalibrationFlags = cms.vint32( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
  mightGet = cms.optional.untracked.vstring
)

process.DQMStore = cms.Service("DQMStore")

process.load("DQMServices.FileIO.DQMFileSaverOnline_cfi")
process.dqmSaver.tag = 'HGCAL'
process.dqmSaver.path = './eos/'
process.dqmSaver.runNumber = 123471

process.p1 = cms.Path(process.playgrounddqmedanalyzer + process.dqmSaver)

# #--------------------------------------------------
# # CM subtraction
# #--------------------------------------------------
# process.playgrounddqmedanalyzer_cm = cms.EDProducer('PlaygroundDQMEDAnalyzer',
#   folder = cms.string('HGCAL/RecHits'),
#   DataType = cms.string('beam'),
#   CalibrationFlags = cms.vint32( 1, 1, 0, 0, 0, 0, 0, 0, 0, 0),
#   mightGet = cms.optional.untracked.vstring
# )
# 
# process.dqmSaver_cm = process.dqmSaver.clone(
#   tag = 'CMTEST',
#   path = './eos/'
# )
# 
# process.p2 = cms.Path(process.playgrounddqmedanalyzer_cm + process.dqmSaver_cm)
