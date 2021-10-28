import FWCore.ParameterSet.Config as cms

from Calibration.LumiAlCaRecoProducers.alcaPCCIntegrator_cfi import alcaPCCIntegrator
alcaPCCProducerRandom = alcaPCCIntegrator.clone()
alcaPCCProducerRandom.inputPccLabel = cms.string("hltAlcaPixelClusterCounts")
alcaPCCProducerRandom.trigstring    = cms.untracked.string("alcaPCCEvent")
alcaPCCProducerRandom.ProdInst      = cms.string("alcaPCCRandom")

# Sequence #

seqALCARECOAlCaPCCRandom = cms.Sequence(alcaPCCProducerRandom)
