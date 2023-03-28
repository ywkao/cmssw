import FWCore.ParameterSet.Config as cms

process = cms.Process("Demo")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents.input = 1
process.source = cms.Source('EmptySource')

#--------------------------------------------------
# EDProducer
#--------------------------------------------------
process.load("Validation.PlaygroundEDProducer.playgroundedproducer_cfi")

## Output definition
#process.out = cms.OutputModule("PoolOutputModule",
#    fileName = cms.untracked.string('file:output/playground_output_RecHits.root'),
#    outputCommands = cms.untracked.vstring('drop *', 'keep *_*_*_Demo'),
#    # type_label_instance_process
#)
#
#process.p = cms.Path(process.playgroundedproducer)
#process.e = cms.EndPath(process.out)

#--------------------------------------------------
# DQM EDAnalyzer
#--------------------------------------------------
#process.load("Validation.PlaygroundEDProducer.testdqmedanalyzer_cfi")
process.testdqmedanalyzer = cms.EDProducer('testDQMEDAnalyzer',
  source = cms.InputTag("playgroundedproducer"),
  folder = cms.string('HGCAL/RecHits'),
  DataType = cms.string('beam'),
  mightGet = cms.optional.untracked.vstring
)

process.DQMStore = cms.Service("DQMStore")

process.load("DQMServices.FileIO.DQMFileSaverOnline_cfi")
process.dqmSaver.tag = 'TEST'
process.dqmSaver.path = './eos/'

#--------------------------------------------------
# Path
#--------------------------------------------------
process.p = cms.Path(process.playgroundedproducer + process.testdqmedanalyzer + process.dqmSaver)
