import FWCore.ParameterSet.Config as cms

# Smoke test of the HGCAL DQM clients: construct, book from the release test
# electronics mapping, and run analyze on events without HGCAL products.
process = cms.Process("TEST")

process.source = cms.Source("EmptySource")
process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(10))

from Geometry.HGCalMapping.hgcalmapping_cff import customise_hgcalmapper
# This test map carries both the DAQ and the trigger (econtidx, trig_fedid) columns.
process = customise_hgcalmapper(process, modules="Geometry/HGCalMapping/data/ModuleMaps/modulelocator_trigger_test.txt")
# The dense-index producers depend on CaloGeometryRecord.
process.load("Configuration.Geometry.GeometryExtendedRun4D104Reco_cff")

process.DQMStore = cms.Service("DQMStore")

process.load("DQM.HGCAL.hgcalDQM_cff")
process.hgcaltpgdqm.SkipTriggerDQM = False

process.p = cms.Path(process.hgcalDQMSources + process.hgcalRecoDQMSources)
