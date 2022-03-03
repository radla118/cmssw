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
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/MakerMacros.h"
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

class RocHitProducer : public edm::stream::EDProducer<> {
public:
  explicit RocHitProducer(const edm::ParameterSet&);
  ~RocHitProducer() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void produce(edm::Event& iEvent, const edm::EventSetup& iSetup) override;

  virtual int indexROC(int irow, int icol, int nROCcolumns) final;

  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> trackerGeometryToken_;
  edm::ESGetToken<TrackerTopology, TrackerTopologyRcd> trackerTopologyToken_;
  edm::ESGetToken<SiPixelFedCablingMap, SiPixelFedCablingMapRcd> siPixelFedCablingMapToken_;

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

RocHitProducer::RocHitProducer(const edm::ParameterSet& iConfig){
  //NOTE: Token for all stream replicas are identical and constructors for the replicas are called
  // sequentially so there is no race condition.
  trackerGeometryToken_ = esConsumes<edm::Transition::Event>();
  trackerTopologyToken_ = esConsumes<edm::Transition::Event>();
  siPixelFedCablingMapToken_ = esConsumes<edm::Transition::Event>();

  /* pixel clusters */
  fPixelClusterLabel_ = iConfig.getParameter<edm::ParameterSet>("RocHitProducerParameters")
                            .getUntrackedParameter<edm::InputTag>("pixelClusterLabel");
  fSiPixelClusterToken_ = consumes<edmNew::DetSetVector<SiPixelCluster>>(fPixelClusterLabel_);

  /* threshold */
  fRocThreshold_ = iConfig.getParameter<edm::ParameterSet>("RocHitProducerParameters").getParameter<double>("RocThreshold");

  /* register products */
  produces<reco::SiRocHits, edm::Transition::Event>("SiRocHits");
}

RocHitProducer::~RocHitProducer() {}

void RocHitProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup){ 
  edm::LogInfo("RocHitProducer") << "start cluster analyzer " << std::endl;

  /* this part could be put in beginRun */
  const TrackerGeometry* trackerGeometry = &iSetup.getData(trackerGeometryToken_);
  const TrackerTopology* trackerTopology = &iSetup.getData(trackerTopologyToken_);
  const SiPixelFedCablingMap* cablingMap = &iSetup.getData(siPixelFedCablingMapToken_);

  std::shared_ptr<SiPixelTopoFinder> pixelTopoFinder = std::make_shared<SiPixelTopoFinder>();
  pixelTopoFinder->init(trackerGeometry, trackerTopology, cablingMap);

  /* update the std::map for pixel geo/topo */
  /* vector of all <int> detIds */
  fDetIds_ = pixelTopoFinder->getDetIds();  //getDetIds();
  /* ROC size (number of row, number of columns for each det id) */
  fSensors_ = pixelTopoFinder->getSensors();
  /* the roc layout on a module */
  fSensorLayout_ = pixelTopoFinder->getSensorLayout();
  /* fedId as a function of detId */
  fFedIds_ = pixelTopoFinder->getFedIds();
  /* map the index ROC to rocId */
  fRocIds_ = pixelTopoFinder->getRocIds();

  auto fDet_ = std::make_unique<reco::SiRocHits>();
  /* count number of events for the current module instance in the luminosityBlock */
  ftotalevents_++;

  /* ----------------------------------------------------------------------
     -- Pixel cluster analysis
     ----------------------------------------------------------------------*/

  edm::Handle<edmNew::DetSetVector<SiPixelCluster>> hClusterColl;
  if (!iEvent.getByToken(fSiPixelClusterToken_, hClusterColl)) {
    edm::LogWarning("RocHitProducer")
        << " edmNew::DetSetVector<SiPixelCluster> " << fPixelClusterLabel_ << " does not exist!" << std::endl;
    return;
  }

  iEvent.getByToken(fSiPixelClusterToken_, hClusterColl);

  if (hClusterColl.isValid()) {
    for (const auto& clusters : *hClusterColl) { /*loop over different clusters in a clusters vector (module)*/
      std::map<int,int> RocAdc; // sum ADC counts for each ROC on the module
      int detid = clusters.detId(); // ID of the module
      for (const auto& clu : clusters) {         /*loop over cluster in a given detId (module)*/
        int rowsperroc = fSensors_[detid].first;
        int colsperroc = fSensors_[detid].second;
        //int nROCrows    = fSensorLayout_[detid].first; //not necessary
        int nROCcolumns = fSensorLayout_[detid].second;

        int roc(-1);
        std::map<int, int> rocIds_detid;
        if (fRocIds_.find(detid) != fRocIds_.end()) {
          rocIds_detid = fRocIds_[detid];
        }

        /* A module is made with a few ROCs
           Need to convert global row/column (on a module) to local row/column (on a ROC) */
        const std::vector<SiPixelCluster::Pixel>& pixvector = clu.pixels();
        for (unsigned int i = 0; i < pixvector.size(); ++i) {
          int mr0 = pixvector[i].x; /* constant column direction is along x-axis */
          int mc0 = pixvector[i].y; /* constant row direction is along y-axis */

          int irow = mr0 / rowsperroc;
          int icol = mc0 / colsperroc;

          int key = indexROC(irow, icol, nROCcolumns);
          if (rocIds_detid.find(key) != rocIds_detid.end()) {
            roc = rocIds_detid[key];
          }
          if(roc > -1) {
            RocAdc[roc] += pixvector[i].adc; /* summing up the ADC values for each ROC */
          }
          //std::cout << " detid " << detid << " roc " << roc << " key " << key << " adc " << pixvector[i].adc << std::endl;
        } /* loop over pixels in a cluster */
      } /* loop over cluster in a detId (module) */
      for(unsigned int i = 0; i < RocAdc.size(); ++i){ //adds the collected ROC ADCs 
        if(RocAdc[i] > fRocThreshold_) {
          //std::cout << " after the sum detid " << detid << " roc " << i << " adc " << RocAdc[i] << std::endl;
          fDet_->fillRocs(detid, i, RocAdc[i]);
        }
      }
    } /* loop over detId-grouped clusters in cluster detId-grouped clusters-vector */
  } /* hClusterColl.isValid() */
  else {
    edm::LogWarning("RocHitProducer")
        << " edmNew::DetSetVector<SiPixelCluster> " << fPixelClusterLabel_ << " is NOT Valid!" << std::endl;
  }
  fDet_->seteventID(ftotalevents_); /* event number is added to the object */
  iEvent.put(std::move(fDet_), std::string("SiRocHits")); /* saving the object */
}

/* helper function */
int RocHitProducer::indexROC(int irow, int icol, int nROCcolumns) {
  return int(icol + irow * nROCcolumns);

  /* generate the folling roc index that is going to map with ROC id as
     8  9  10 11 12 13 14 15
     0  1  2  3  4  5  6  7 */
}

  /* module description */
void RocHitProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  {
    edm::ParameterSetDescription psd0;
    psd0.addUntracked<edm::InputTag>("pixelClusterLabel", edm::InputTag("siPixelClusters", "", "RECO"));
    psd0.add<double>("RocThreshold",1);
    psd0.add<std::vector<edm::InputTag>>("badPixelFEDChannelCollections",
                                         {
                                             edm::InputTag("siPixelDigis"),
                                         });
    desc.add<edm::ParameterSetDescription>("RocHitProducerParameters", psd0);
  }
  descriptions.add("rocHitProducer", desc);
}
DEFINE_FWK_MODULE(RocHitProducer);
