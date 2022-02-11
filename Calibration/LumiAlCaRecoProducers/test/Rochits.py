import os
import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Run2_2018_cff import Run2_2018
process = cms.Process('PCL',Run2_2018)

# ----------------------------------------------------------------------
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.threshold = 'INFO'
process.MessageLogger.cerr.FwkReport.reportEvery = 10000
#process.MessageLogger.categories.append('HLTrigReport')
#process.MessageLogger.categories.append('L1GtTrigReport')
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
#process.GlobalTag = GlobalTag(process.GlobalTag, '92X_dataRun2_Express_v8', '')
process.GlobalTag = GlobalTag(process.GlobalTag, '101X_dataRun2_Express_v8', '')
#process.GlobalTag = GlobalTag(process.GlobalTag, '113X_dataRun3_Express_Candidate_2021_06_21_16_58_57', '')
# -- Input files
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
    #"/store/express/Run2017F/ExpressPhysics/FEVT/Express-v1/000/305/366/00000/863EC350-6EB6-E711-8EAD-02163E019B61.root",
    #"/store/express/Run2017F/ExpressPhysics/FEVT/Express-v1/000/305/366/00000/B6268B1F-6FB6-E711-A46C-02163E01439D.root",
    #"/store/express/Run2018E/ExpressPhysics/FEVT/Express-v1/000/325/283/00000/DF31860D-3EA3-2B42-A7CB-ACFE8C2810DF.root",
    #"/store/express/Run2018E/ExpressPhysics/FEVT/Express-v1/000/325/284/00000/3E6464E9-5FB6-0646-98EE-BE547936D8A5.root"
    "/store/express/Run2018E/ExpressPhysics/FEVT/Express-v1/000/325/308/00000/11F85965-DBA3-624B-A5C4-A42E89D768D7.root"
    ),
    #lumisToProcess = cms.untracked.VLuminosityBlockRange("305366:1-305366:1"),
)


# -- number of events
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(2)
)

#from EventFilter.SiPixelRawToDigi.SiPixelRawToDigi_cfi import siPixelDigis
#process.siPixelDigis = siPixelDigis.clone()
#process.siPixelDigis.InputLabel = cms.InputTag("rawDataCollector")

process.SiRocHitProducer = cms.EDProducer("SiRocHitProducer",
    SiRocHitProducerParameters = cms.PSet(
        #badPixelFEDChannelCollections = cms.VInputTag(cms.InputTag('siPixelDigis')),
        pixelClusterLabel = cms.untracked.InputTag("siPixelClusters::RECO"),
        #monitorOnDoubleColumn = cms.untracked.bool(False),
        #resetEveryNLumi = cms.untracked.int32( 1 )
    )
)

process.ALCARECOStreamRocHits = cms.OutputModule("PoolOutputModule",
    fileName = cms.untracked.string('RocHits.root'),
    outputCommands = cms.untracked.vstring('drop *',
        'keep *_SiRocHitProducer_*_*',
        'keep *_siPixelClusters_*_*',
    )
)

#process.p = cms.Path(process.siPixelDigis*process.SiRocHitProducer)
process.p = cms.Path(process.SiRocHitProducer)
process.end = cms.EndPath(process.ALCARECOStreamRocHits)

process.schedule = cms.Schedule(process.p,process.end)

# Add early deletion of temporary data products to reduce peak memory need
from Configuration.StandardSequences.earlyDeleteSettings_cff import customiseEarlyDelete
process = customiseEarlyDelete(process)
# End adding early deletion

