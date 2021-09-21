import os
import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Run2_2018_cff import Run2_2018
process = cms.Process('PCL',Run2_2018)

# ----------------------------------------------------------------------
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.threshold = 'INFO'
process.MessageLogger.cerr.FwkReport.reportEvery = 10000
process.options = cms.untracked.PSet(
    SkipEvent = cms.untracked.vstring('ProductNotFound'),
    wantSummary = cms.untracked.bool(True)
)

# -- Conditions
process.load("Configuration.StandardSequences.MagneticField_38T_cff")
process.load("Configuration.StandardSequences.GeometryRecoDB_cff")
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load('EventFilter.SiPixelRawToDigi.SiPixelRawToDigi_cfi')

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '101X_dataRun2_Express_v8', '')
# -- Input files
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
    "/store/express/Run2018E/ExpressPhysics/FEVT/Express-v1/000/325/308/00000/11F85965-DBA3-624B-A5C4-A42E89D768D7.root"
    ),
)


# -- number of events
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(100)
)

#from EventFilter.SiPixelRawToDigi.SiPixelRawToDigi_cfi import siPixelDigis
#process.siPixelDigis = siPixelDigis.clone()
#process.siPixelDigis.InputLabel = cms.InputTag("rawDataCollector")

process.SiRocHitProducer = cms.EDProducer("SiRocHitProducer",
    SiRocHitProducerParameters = cms.PSet(
        #input PCC label
        pixelClusterLabel = cms.untracked.InputTag("siPixelClusters::RECO"),
        #sum charge/ROC threshold
        RocThreshold = cms.double(0.0)
    )
)

process.ALCARECOStreamRocHits = cms.OutputModule("PoolOutputModule",
    fileName = cms.untracked.string('SiRocSumChargePerEvent_100.root'),
    outputCommands = cms.untracked.vstring('drop *',
        'keep *_SiRocHitProducer_*_*',
        'keep *_siPixelClusters_*_*',
    )
)

process.p = cms.Path(process.SiRocHitProducer)
process.end = cms.EndPath(process.ALCARECOStreamRocHits)

process.schedule = cms.Schedule(process.p,process.end)

# Add early deletion of temporary data products to reduce peak memory need
from Configuration.StandardSequences.earlyDeleteSettings_cff import customiseEarlyDelete
process = customiseEarlyDelete(process)
# End adding early deletion

