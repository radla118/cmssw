import FWCore.ParameterSet.Config as cms

from Calibration.LumiAlCaRecoProducers.alcaPCCIntegrator_cfi import alcaPCCIntegrator
alcaPCCProducerRandomFromRECO = alcaPCCIntegrator.clone()
alcaPCCProducerRandomFromRECO.inputPccLabel = cms.InputTag("hltAlcaPixelClusterCounts") #HLT input tag
alcaPCCProducerRandomFromRECO.trigstring    = cms.untracked.string("alcaPCCEvent")
alcaPCCProducerRandomFromRECO.prodInst      = cms.untracked.string("alcaPCCRandomFromRECO")

# Sequence #
seqALCARECOAlCaPCCRandomFromRECO = cms.Sequence(alcaPCCProducerRandomFromRECO)
