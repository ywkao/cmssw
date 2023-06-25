import FWCore.ParameterSet.Config as cms

process = cms.Process("Demo")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents.input = 1
process.source = cms.Source('EmptySource')

process.hgcaltestbeamclient = cms.EDProducer('HGCalTestBeamClient',
  folder = cms.string('HGCAL/Digis'),
  DataType = cms.string('beam'),
  #DataType = cms.string('pedestal'),
  CalibrationFlags = cms.vint32( 1, 0, 0, 0, 0, 0, 0, 0, 0, 0),
  TB2022beamData = cms.string("/eos/cms/store/group/dpg_hgcal/tb_hgcal/2022/sps_oct2022/pion_beam_150_320fC/beam_run/run_20221007_191926/beam_run0.root"),
  TB2022pedData = cms.string("/eos/cms/store/group/dpg_hgcal/tb_hgcal/2022/sps_oct2022/pedestals/pedestal_320fC/pedestal_run/run_20221008_192720/pedestal_run0.root"),
  mightGet = cms.optional.untracked.vstring
)

process.DQMStore = cms.Service("DQMStore")

process.load("DQMServices.FileIO.DQMFileSaverOnline_cfi")
process.dqmSaver.tag = 'HGCAL'
#process.dqmSaver.path = './eos/'
process.dqmSaver.runNumber = 123472

process.p1 = cms.Path(process.hgcaltestbeamclient + process.dqmSaver)
