import FWCore.ParameterSet.Config as cms

process = cms.Process('NANO')
process.source = cms.Source("PoolSource", fileNames = cms.untracked.vstring())

process.source.fileNames = [
	'root://cms-xrd-global.cern.ch//store/data/Run2016F/SingleMuon/NANOAOD/05Feb2018-v1/80000/60C8BE77-690C-E811-A28F-0CC47A1E0466.root' ##you can change only this line
]

process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(10))

process.output = cms.OutputModule("PoolOutputModule", fileName = cms.untracked.string('tree.root'))
process.out = cms.EndPath(process.output)
