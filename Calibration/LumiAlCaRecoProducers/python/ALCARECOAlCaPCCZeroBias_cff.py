import FWCore.ParameterSet.Config as cms

from Calibration.LumiAlCaRecoProducers.alcaPCCIntegrator_cfi import alcaPCCIntegrator
alcaPCCProducerZeroBias = alcaPCCIntegrator.clone()
alcaPCCProducerZeroBias.inputPccLabel = cms.string("hltAlcaPixelClusterCounts")
alcaPCCProducerZeroBias.trigstring    = cms.untracked.string("alcaPCCEvent")
alcaPCCProducerZeroBias.ProdInst      = cms.string("alcaPCCZeroBias")

# Sequence #

seqALCARECOAlCaPCCZeroBias = cms.Sequence(alcaPCCProducerZeroBias)
