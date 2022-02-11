// C++ standard
#include <string>
// ROOT
#include "TMath.h"
// CMSSW FW
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
// CMSSW DataFormats
#include "DataFormats/Common/interface/ConditionsInEdm.h"
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"
#include "DataFormats/SiPixelCluster/interface/SiPixelCluster.h"
#include "DataFormats/TrackerCommon/interface/PixelBarrelName.h"
#include "DataFormats/TrackerCommon/interface/PixelEndcapName.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
// "FED error 25"
#include "DataFormats/SiPixelDetId/interface/PixelFEDChannel.h"
// CMSSW CondFormats
#include "CondFormats/RunInfo/interface/RunSummary.h"
#include "CondFormats/RunInfo/interface/RunInfo.h"
#include "CondFormats/DataRecord/interface/RunSummaryRcd.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelFrameConverter.h"
#include "Geometry/CommonDetUnit/interface/PixelGeomDetUnit.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"

// Header file
#include "CalibTracker/SiPixelQuality/plugins/SiRocHitProducer.h"

#include "CalibTracker/SiPixelQuality/plugins/RocHits.h"

SiRocHitProducer::SiRocHitProducer(const edm::ParameterSet& iConfig, SiPixelStatusCache const* iCache) {
  //NOTE: Token for all stream replicas are identical and constructors for the replicas are called
  // sequentially so there is no race condition.
  iCache->trackerGeometryToken_ = esConsumes<edm::Transition::BeginRun>();
  iCache->trackerTopologyToken_ = esConsumes<edm::Transition::BeginRun>();
  iCache->siPixelFedCablingMapToken_ = esConsumes<edm::Transition::BeginRun>();

  /* pixel clusters */
  fPixelClusterLabel_ = iConfig.getParameter<edm::ParameterSet>("SiRocHitProducerParameters")
                            .getUntrackedParameter<edm::InputTag>("pixelClusterLabel");
  fSiPixelClusterToken_ = consumes<edmNew::DetSetVector<SiPixelCluster>>(fPixelClusterLabel_);

  //debug_ = iConfig.getUntrackedParameter<bool>("debug");

  /* register products */
  produces<reco::RocHits, edm::Transition::Event>("siRocHits");
}

SiRocHitProducer::~SiRocHitProducer() {}

//--------------------------------------------------------------------------------------------------

std::shared_ptr<SiPixelTopoFinder> SiRocHitProducer::globalBeginRun(edm::Run const& iRun,
                                                                         edm::EventSetup const& iSetup,
                                                                         GlobalCache const* iCache) {
  const TrackerGeometry* trackerGeometry = &iSetup.getData(iCache->trackerGeometryToken_);
  const TrackerTopology* trackerTopology = &iSetup.getData(iCache->trackerTopologyToken_);
  const SiPixelFedCablingMap* cablingMap = &iSetup.getData(iCache->siPixelFedCablingMapToken_);

  auto returnValue = std::make_shared<SiPixelTopoFinder>();

  returnValue->init(trackerGeometry, trackerTopology, cablingMap);
  return returnValue;
}

void SiRocHitProducer::beginRun(edm::Run const&, edm::EventSetup const&) {
  /*Is it good to pass the objects stored in runCache to set class private members values  *
     or just call runCahche every time in the calss function?*/

  edm::LogInfo("SiRocHitProducer") << "beginRun: update the std::map for pixel geo/topo " << std::endl;
  /* update the std::map for pixel geo/topo */
  /* vector of all <int> detIds */
  fDetIds_ = runCache()->getDetIds();  //getDetIds();
  /* ROC size (number of row, number of columns for each det id) */
  fSensors_ = runCache()->getSensors();
  /* the roc layout on a module */
  fSensorLayout_ = runCache()->getSensorLayout();
  /* fedId as a function of detId */
  fFedIds_ = runCache()->getFedIds();
  /* map the index ROC to rocId */
  fRocIds_ = runCache()->getRocIds();
}

void SiRocHitProducer::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) {
  //edm::LogInfo("SiRocHitProducer") << "beginlumi instance" << std::endl;

  /* initialize fDet_ with a set of modules(detIds) and clean the fFEDerror25_ */
  //fDet_ = reco::RocHits();
  //for (unsigned int itDetId = 0; itDetId < fDetIds_.size(); ++itDetId) {
  //  int detid = fDetIds_[itDetId];
  //  int nrocs = fSensorLayout_[detid].first * fSensorLayout_[detid].second;

  //  fDet_.addModule(detid, nrocs);
  //}

  //fFEDerror25_.clear();
  //ftotalevents_ = 0;
}

void SiRocHitProducer::produce(edm::Event& iEvent, edm::EventSetup const&) {
  edm::LogInfo("SiRocHitProducer") << "start cluster analyzer " << std::endl;

  auto fDet_ = std::make_unique<reco::RocHits>();
 for (unsigned int itDetId = 0; itDetId < fDetIds_.size(); ++itDetId) {
    int detid = fDetIds_[itDetId];
    int nrocs = fSensorLayout_[detid].first * fSensorLayout_[detid].second;
  }

  /* count number of events for the current module instance in the luminosityBlock */
  ftotalevents_++;

  /* ----------------------------------------------------------------------
     -- Pixel cluster analysis
     ----------------------------------------------------------------------*/

  edm::Handle<edmNew::DetSetVector<SiPixelCluster>> hClusterColl;
  if (!iEvent.getByToken(fSiPixelClusterToken_, hClusterColl)) {
    edm::LogWarning("SiRocHitProducer")
        << " edmNew::DetSetVector<SiPixelCluster> " << fPixelClusterLabel_ << " does not exist!" << std::endl;
    return;
  }

  iEvent.getByToken(fSiPixelClusterToken_, hClusterColl);

  if (hClusterColl.isValid()) {
    for (const auto& clusters : *hClusterColl) { /*loop over different clusters in a clusters vector (module)*/
      for (const auto& clu : clusters) {         /*loop over cluster in a given detId (module)*/
        int detid = clusters.detId();
        int rowsperroc = fSensors_[detid].first;
        int colsperroc = fSensors_[detid].second;
        //int nROCrows    = fSensorLayout_[detid].first;
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
          //std::err << "event " << ftotalevents_ << " detid " << detid << " roc " << roc << std::endl;

          fDet_->fillRocs(detid, roc);

        } /* loop over pixels in a cluster */

      } /* loop over cluster in a detId (module) */

    } /* loop over detId-grouped clusters in cluster detId-grouped clusters-vector* */

  } /* hClusterColl.isValid() */
  else {
    edm::LogWarning("SiRocHitProducer")
        << " edmNew::DetSetVector<SiPixelCluster> " << fPixelClusterLabel_ << " is NOT Valid!" << std::endl;
  }
  fDet_->seteventID(ftotalevents_);
  iEvent.put(std::move(fDet_), std::string("siRocHits"));
}



void SiRocHitProducer::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) {
}

/* helper function */
int SiRocHitProducer::indexROC(int irow, int icol, int nROCcolumns) {
  return int(icol + irow * nROCcolumns);

  /* generate the folling roc index that is going to map with ROC id as
     8  9  10 11 12 13 14 15
     0  1  2  3  4  5  6  7 */
}

DEFINE_FWK_MODULE(SiRocHitProducer);
