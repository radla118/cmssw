/*_________________________________________________________________
class:   AlcaPCCIntegrator.cc



authors: Sam Higginbotham (shigginb@cern.ch), Chris Palmer (capalmer@cern.ch), Attila Radl (attila.radl@cern.ch) 

________________________________________________________________**/

// C++ standard
#include <string>
// CMS
#include "DataFormats/Luminosity/interface/PixelClusterCountsInEvent.h"
#include "DataFormats/Luminosity/interface/PixelClusterCounts.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "TMath.h"

#include "DataFormats/DetId/interface/DetId.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
//#include "FWCore/Framework/interface/one/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

#include "FWCore/Utilities/interface/ReusableObjectHolder.h"
#include "FWCore/Concurrency/interface/SerialTaskQueue.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/Utilities/interface/ESGetToken.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESWatcher.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "FWCore/Utilities/interface/Transition.h"
#include "FWCore/Utilities/interface/ESGetToken.h"

struct AlcaPCCIntegratorGlobalCache {
  mutable edm::EDPutTokenT<reco::PixelClusterCounts> lumiPutToken_;
};

//The class
class AlcaPCCIntegrator
    : public edm::stream::EDProducer<edm::LuminosityBlockSummaryCache<std::vector<reco::PixelClusterCountsInEvent>>, 
	                                                edm::GlobalCache<AlcaPCCIntegratorGlobalCache>,
                                                        edm::EndLuminosityBlockProducer,
							edm::Accumulator> {
public:
  //explicit AlcaPCCIntegrator(const edm::ParameterSet&);

  AlcaPCCIntegrator(const edm::ParameterSet& iConfig, AlcaPCCIntegratorGlobalCache const* iCache) {
    pccSource_ =
        iConfig.getParameter<edm::ParameterSet>("AlcaPCCIntegratorParameters").getParameter<std::string>("inputPccLabel");
    auto trigstring_ = iConfig.getParameter<edm::ParameterSet>("AlcaPCCIntegratorParameters")
                           .getUntrackedParameter<std::string>("trigstring", "alcaPCC");
    std::string prodInst_ =
        iConfig.getParameter<edm::ParameterSet>("AlcaPCCIntegratorParameters").getParameter<std::string>("ProdInst");

    edm::InputTag PCCInputTag_(pccSource_, trigstring_);

    countLumi_ = 0;
    lumiPutToken_ = produces<reco::PixelClusterCounts, edm::Transition::EndLuminosityBlock>(prodInst_);
    iCache->lumiPutToken_ = lumiPutToken_;
    pccToken_ = consumes<reco::PixelClusterCountsInEvent>(PCCInputTag_);
}

  ~AlcaPCCIntegrator() override;

//for global cache
  static std::unique_ptr<AlcaPCCIntegratorGlobalCache> initializeGlobalCache(edm::ParameterSet const& iPSet) {
    return std::make_unique<AlcaPCCIntegratorGlobalCache>();
  }

  static void globalEndJob(AlcaPCCIntegratorGlobalCache const*) { /* Do nothing */
  
  }

  static std::shared_ptr<std::vector<reco::PixelClusterCountsInEvent>> globalBeginLuminosityBlockSummary(edm::LuminosityBlock const&, edm::EventSetup const&, LuminosityBlockContext const*);
  void beginLuminosityBlockSummary(edm::LuminosityBlock const&, edm::EventSetup const&);
  //void produce(edm::Event& iEvent, edm::EventSetup const&);
  void accumulate(const edm::Event& iEvent, const edm::EventSetup&);
  void endLuminosityBlockSummary(const edm::LuminosityBlock&, const edm::EventSetup&, std::vector<reco::PixelClusterCountsInEvent>* iCounts) const {
    for(unsigned int itr = 0; itr < thePCCob.size(); ++itr){
      iCounts->push_back(thePCCob[itr]);
    }
    //thePCCob.clear();
  }

  static void globalEndLuminosityBlockSummary(edm::LuminosityBlock const&,
                                            edm::EventSetup const&,
                                            LuminosityBlockContext const* iContext,
                                            std::vector<reco::PixelClusterCountsInEvent>* iCounts) {};
  static void globalEndLuminosityBlockProduce(edm::LuminosityBlock& iLumi,
                                           edm::EventSetup const&,
                                           LuminosityBlockContext const* iContext,
					   std::vector<reco::PixelClusterCountsInEvent> const* iCounts) {
  std::unique_ptr<reco::PixelClusterCounts>  lumiPCC = std::make_unique<reco::PixelClusterCounts>();
    for(unsigned int i = 0; i < iCounts->size(); ++i){
      lumiPCC->add((*iCounts)[i]);
    }
    //Saving the PCC object
    iLumi.put(iContext->global()->lumiPutToken_, std::move(lumiPCC));
  }

private:
  edm::EDGetTokenT<reco::PixelClusterCountsInEvent> pccToken_;
  edm::EDPutTokenT<reco::PixelClusterCounts> lumiPutToken_;
  std::string pccSource_;

  int countEvt_;            //counter
  int countLumi_;           //counter

  std::vector<reco::PixelClusterCountsInEvent> thePCCob;
  std::vector<reco::PixelClusterCountsInEvent>* iCounts;
};


//--------------------------------------------------------------------------------------------------
AlcaPCCIntegrator::~AlcaPCCIntegrator() {}

std::shared_ptr<std::vector<reco::PixelClusterCountsInEvent>> AlcaPCCIntegrator::globalBeginLuminosityBlockSummary(
                                                                                  edm::LuminosityBlock const&,
                                                                                  edm::EventSetup const&,
                                                                                  LuminosityBlockContext const*) {
  return std::make_shared<std::vector<reco::PixelClusterCountsInEvent>>();
}

void AlcaPCCIntegrator::beginLuminosityBlockSummary(edm::LuminosityBlock const&, edm::EventSetup const&) {
  //PCC object at the beginning of each lumi section
  // do nothing
  thePCCob.clear();
  countLumi_++;
}

void AlcaPCCIntegrator::accumulate(const edm::Event& iEvent, const edm::EventSetup&) {
  countEvt_++;
  //Looping over the clusters and adding the counts up
  edm::Handle<reco::PixelClusterCountsInEvent> pccHandle;
  iEvent.getByToken(pccToken_, pccHandle);

  const reco::PixelClusterCountsInEvent inputPcc = *(pccHandle.product());
  //thePCCob->add(inputPcc);
  thePCCob.push_back(inputPcc);
}
/*
void AlcaPCCIntegrator::endLuminosityBlockSummary(const edm::LuminosityBlock&, const edm::EventSetup&, std::vector<reco::PixelClusterCountsInEvent>* iCounts) {
  //add the Stream's partial information to the full information
  //iCounts->insert(thePCCob.begin(),thePCCob.end());
  //iCounts->push_back(thePCCob);
  for(unsigned int itr = 0; itr < thePCCob.size(); ++itr){
    iCounts->push_back(thePCCob[itr]);
  }
  //thePCCob.clear();
}

void AlcaPCCIntegrator::globalEndLuminosityBlockSummary(edm::LuminosityBlock& iLumi, 
                                            edm::EventSetup const&, 
                                            LuminosityBlockContext const* iContext, 
                                            std::vector<reco::PixelClusterCountsInEvent>* iCounts) {
  //Nothing to do
}

void AlcaPCCIntegrator::globalEndLuminosityBlockProduce(edm::LuminosityBlock& iLumi, 
                                           edm::EventSetup const&, 
                                           LuminosityBlockContext const* iContext, 
                                           std::vector<reco::PixelClusterCountsInEvent> const* iCounts) {
  auto lumiPCC = std::make_unique<reco::PixelClusterCounts>();
  for(unsigned int i = 0; i < iCounts->size(); ++i){
    lumiPCC->add((*iCounts)[i]);
  }	  
  //Saving the PCC object
  //iLumi.put(std::move(lumiPCC), std::string(prodInst_));
  iLumi.put(lumiPCC);
}
*/
DEFINE_FWK_MODULE(AlcaPCCIntegrator);
