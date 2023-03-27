import FWCore.ParameterSet.Config as cms

playgroundedproducer = cms.EDProducer('PlaygroundEDProducer',
  folder = cms.string('HGCAL/RecHits'),
  DataType = cms.string('beam'),
  CalibrationFlags = cms.vint32(
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  ),
  CalibrationCSVFile = cms.string('./meta_conditions/calibration_parameters.csv'),
  mightGet = cms.optional.untracked.vstring
)
