import FWCore.ParameterSet.Config as cms

import HLTrigger.HLTfilters.hltHighLevel_cfi
ALCARECORandomHLT = HLTrigger.HLTfilters.hltHighLevel_cfi.hltHighLevel.clone(
    HLTPaths = cms.vstring("*Random*"),
    eventSetupPathsKey='',
    TriggerResultsTag = cms.InputTag("TriggerResults","","HLT"),
    andOr = cms.bool(True), # choose logical OR between Triggerbits
    throw = cms.bool(False) # tolerate triggers stated above, but not available
)

from Calibration.LumiAlCaRecoProducers.alcaPCCIntegrator_cfi import alcaPCCIntegrator
alcaPCCProducerRandom = alcaPCCIntegrator.clone()
alcaPCCProducerRandom.inputPccLabel = cms.string("hltAlcaPixelClusterCounts")
alcaPCCProducerRandom.trigstring    = cms.untracked.string("alcaPCCRandom")
alcaPCCProducerRandom.ProdInst      = cms.string("alcaPCCRandom")

# Sequence #
seqALCARECOAlCaPCCRandom = cms.Sequence(ALCARECORandomHLT+alcaPCCProducerRandom)
