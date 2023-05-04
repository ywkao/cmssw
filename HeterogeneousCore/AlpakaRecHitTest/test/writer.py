import FWCore.ParameterSet.Config as cms

process = cms.Process('Writer')

process.source = cms.Source('EmptySource')

process.load('Configuration.StandardSequences.Accelerators_cff')
process.load('HeterogeneousCore.AlpakaCore.ProcessAcceleratorAlpaka_cfi')

#--------------------------------------------------
# Message logger
#--------------------------------------------------
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = cms.untracked.int32(1)
process.MessageLogger.cerr.INFO.limit = cms.untracked.int32(10000000)

#-------------------------------------------------------------------------------------
# running on the best backend, i.e.
# run the producer on a gpu and copy the product to the cpu, or run directly on the cpu
#-------------------------------------------------------------------------------------
process.testProducer = cms.EDProducer('AlpakaRecHitTest@alpaka',
    size = cms.int32(42)
)

process.testAnalyzer = cms.EDAnalyzer('TestAlpakaRecHitAnalyzer',
    source = cms.InputTag('testProducer')
)

#--------------------------------------------------
# run the producer explicitly on the cpu
#--------------------------------------------------
process.testProducerSerial = cms.EDProducer('alpaka_serial_sync::AlpakaRecHitTest',
    size = cms.int32(42)
)

process.testAnalyzerSerial = cms.EDAnalyzer('TestAlpakaRecHitAnalyzer',
    source = cms.InputTag('testProducerSerial')
)

#--------------------------------------------------
# write the product to a 'output_test.root' file
#--------------------------------------------------
process.output = cms.OutputModule('PoolOutputModule',
    fileName = cms.untracked.string('output_test.root'),
    outputCommands = cms.untracked.vstring(
        'drop *',
        #'keep *_testProducer_*_*',
        'keep *_testProducerSerial_*_*',
    )
)

#process.gpu_path = cms.Path(process.testProducer + process.testAnalyzer)
process.serial_path = cms.Path(process.testProducerSerial + process.testAnalyzerSerial)
process.output_path = cms.EndPath(process.output)

process.maxEvents.input = 10
