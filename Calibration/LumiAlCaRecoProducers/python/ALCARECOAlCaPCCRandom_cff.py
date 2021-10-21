import FWCore.ParameterSet.Config as cms

import HLTrigger.HLTfilters.hltHighLevel_cfi
ALCARECORandomHLT = HLTrigger.HLTfilters.hltHighLevel_cfi.hltHighLevel.clone(
    HLTPaths = cms.vstring("*Random*"),
    eventSetupPathsKey='',
    TriggerResultsTag = cms.InputTag("TriggerResults","","HLT"),
    andOr = cms.bool(True), # choose logical OR between Triggerbits
    throw = cms.bool(False) # tolerate triggers stated above, but not available
)

from EventFilter.SiPixelRawToDigi.SiPixelRawToDigi_cfi import siPixelDigis
siPixelDigisForLumiR = siPixelDigis.cpu.clone(
    InputLabel = "hltFEDSelectorLumiPixels"
)

from RecoLocalTracker.SiPixelClusterizer.SiPixelClusterizerPreSplitting_cfi import siPixelClustersPreSplitting
siPixelClustersForLumiR = siPixelClustersPreSplitting.cpu.clone(
    src = "siPixelDigisForLumiR"
)

from Calibration.LumiAlCaRecoProducers.alcaPCCEventProducer_cfi import alcaPCCEventProducer
alcaPCCEventProducerRandom = alcaPCCEventProducer.clone()
alcaPCCEventProducerRandom.pixelClusterLabel = cms.InputTag("siPixelClustersForLumiR")
alcaPCCEventProducerRandom.trigstring        = cms.untracked.string("alcaPCCRandom")

from Calibration.LumiAlCaRecoProducers.alcaPCCIntegrator_cfi import alcaPCCIntegrator
alcaPCCProducerRandom = alcaPCCIntegrator.clone()
alcaPCCProducerRandom.inputPccLabel = cms.string("alcaPCCEventProducerRandom")
alcaPCCProducerRandom.trigstring    = cms.untracked.string("alcaPCCRandom")
alcaPCCProducerRandom.ProdInst      = cms.string("alcaPCCRandom")

# Sequence #
seqALCARECOAlCaPCCRandom = cms.Sequence(ALCARECORandomHLT + siPixelDigisForLumiR + siPixelClustersForLumiR + alcaPCCEventProducerRandom + alcaPCCProducerRandom)
