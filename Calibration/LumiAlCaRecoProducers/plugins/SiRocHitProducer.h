#ifndef Calibration_LumiAlCaRecoProducers_SiRocHitProducer_h
#define Calibration_LumiAlCaRecoProducers_SiRocHitProducer_h

/**_________________________________________________________________
 *    class:   SiRocHitProducer.h
 *       package: Calibration/LumiAlCaRecoProducers 
 *________________________________________________________________**/

// C++ standard
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
// // CMS FW
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESWatcher.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "FWCore/Utilities/interface/Transition.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
// Concurrency
#include "FWCore/Utilities/interface/ReusableObjectHolder.h"
#include "FWCore/Concurrency/interface/SerialTaskQueue.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
// // Pixel data format
#include "CalibTracker/SiPixelQuality/interface/SiPixelDetectorStatus.h"
#include "CondFormats/DataRecord/interface/SiPixelFedCablingMapRcd.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelFedCablingMap.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/Common/interface/DetSetVectorNew.h"
//SiRocHits data format
#include "DataFormats/Luminosity/interface/SiRocHits.h"
// Tracker Geo
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
// SiPixelTopoFinder
#include "CalibTracker/SiPixelQuality/interface/SiPixelTopoFinder.h"
// SiPixelDetectorStatus
#include "CalibTracker/SiPixelQuality/interface/SiPixelDetectorStatus.h"

using namespace std;

/* Cache to pertain SiPixelTopoFinder */
class SiPixelStatusCache {
public:
  //NOTE: these are only changes in the constructor call
  mutable edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> trackerGeometryToken_;
  mutable edm::ESGetToken<TrackerTopology, TrackerTopologyRcd> trackerTopologyToken_;
  mutable edm::ESGetToken<SiPixelFedCablingMap, SiPixelFedCablingMapRcd> siPixelFedCablingMapToken_;
};

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class SiRocHitProducer : public edm::stream::EDProducer<edm::GlobalCache<SiPixelStatusCache>,
                                edm::RunCache<SiPixelTopoFinder>> {

public:
  SiRocHitProducer(edm::ParameterSet const& iPSet, SiPixelStatusCache const*);
  ~SiRocHitProducer() override;

  /* module description */
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    {
      edm::ParameterSetDescription psd0;
      psd0.addUntracked<edm::InputTag>("pixelClusterLabel", edm::InputTag("siPixelClusters", "", "RECO"));
      psd0.add<double>("RocThreshold",1);
      psd0.add<std::vector<edm::InputTag>>("badPixelFEDChannelCollections",
                                           {
                                               edm::InputTag("siPixelDigis"),
                                           });
      desc.add<edm::ParameterSetDescription>("SiRocHitProducerParameters", psd0);
    }
    descriptions.add("siRocHitProducer", desc);
  }

  /*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

  /* For each instance of the module*/
  void beginRun(edm::Run const&, edm::EventSetup const&) final;

  void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) final;
  void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) final;

  static std::unique_ptr<SiPixelStatusCache> initializeGlobalCache(edm::ParameterSet const& iPSet) {
    edm::LogInfo("SiRocHitProducer") << "Init global Cache " << std::endl;
    return std::make_unique<SiPixelStatusCache>();
  }

  static std::shared_ptr<SiPixelTopoFinder> globalBeginRun(edm::Run const& iRun,
                                                           edm::EventSetup const& iSetup,
                                                           GlobalCache const* iCache);

  static void globalEndRun(edm::Run const& iRun, edm::EventSetup const&, RunContext const* iContext) {
    /* Do nothing */
  }

  static void globalEndJob(SiPixelStatusCache const*) {
    /* Do nothing */
  }

  static void globalEndLuminosityBlockProduce(edm::LuminosityBlock& iLumi,
                                              edm::EventSetup const&,
                                              LuminosityBlockContext const* iContext) {
   /* Do nothing */
  }

  void produce(edm::Event& iEvent, edm::EventSetup const&) override;

private:
  virtual int indexROC(int irow, int icol, int nROCcolumns) final;

  /* ParameterSet */
  static const bool debug_ = false;

  edm::InputTag fPixelClusterLabel_;
  edm::EDGetTokenT<edmNew::DetSetVector<SiPixelCluster>> fSiPixelClusterToken_;

  double fRocThreshold_;

  /*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
  /* private data member, one instance per stream */

  /* per-Run data (The pixel topo cannot be changed during a Run) */
  /* vector of all <int> detIds */
  std::vector<int> fDetIds_;
  /* ROC size (number of row, number of columns for each det id) */
  std::map<int, std::pair<int, int>> fSensors_;
  /* the roc layout on a module */
  std::map<int, std::pair<int, int>> fSensorLayout_;
  /* fedId as a function of detId */
  std::unordered_map<uint32_t, unsigned int> fFedIds_;
  /* map the index ROC to rocId */
  std::map<int, std::map<int, int>> fRocIds_;

  /* per Run data */
  unsigned long int ftotalevents_;

  int beginRun_;
  int endRun_;

  // Producer production (output collection)
  std::unique_ptr<reco::SiRocHits> fDet_;
};
#endif
