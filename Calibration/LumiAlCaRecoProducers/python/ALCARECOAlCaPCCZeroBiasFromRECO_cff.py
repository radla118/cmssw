import FWCore.ParameterSet.Config as cms

from Calibration.LumiAlCaRecoProducers.alcaPCCIntegrator_cfi import alcaPCCIntegrator
alcaPCCProducerZBFromRECO = alcaPCCIntegrator.clone()
alcaPCCProducerZBFromRECO.inputPccLabel = cms.InputTag("hltAlcaPixelClusterCounts") #HLT input tag
alcaPCCProducerZBFromRECO.trigstring    = cms.untracked.string("alcaPCCEvent")
alcaPCCProducerZBFromRECO.prodInst      = cms.untracked.string("alcaPCCZeroBiasFromRECO")

# Sequence #
seqALCARECOAlCaPCCZBFromRECO = cms.Sequence(alcaPCCProducerZBFromRECO)
