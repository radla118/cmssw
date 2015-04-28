//
// Derived from HLTrigger/special/src/HLTPixelClusterShapeFilter.cc
// at version 7_5_0_pre3
//
// Original Author (of Derivative Producer):  Eric Appelt
//         Created:  Mon Apr 27, 2015

#include <tuple>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/SiPixelDetId/interface/PixelSubdetector.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHitCollection.h"
#include "DataFormats/HeavyIonEvent/interface/ClusterCompatibility.h"

#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "Geometry/CommonTopologies/interface/PixelTopology.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"


//
// class declaration
//

class ClusterCompatibilityProducer : public edm::EDProducer {

  public:
    explicit ClusterCompatibilityProducer(const edm::ParameterSet&);
    ~ClusterCompatibilityProducer();

  private:
    virtual void beginJob() override ;
    virtual void produce(edm::Event&, const edm::EventSetup&) override;
    virtual void endJob() override ;

    edm::EDGetTokenT<SiPixelRecHitCollection> inputToken_;
    edm::InputTag       inputTag_;      // input tag identifying product containing pixel clusters
    double              minZ_;          // beginning z-vertex position
    double              maxZ_;          // end z-vertex position
    double              zStep_;         // size of steps in z-vertex test

    struct VertexHit
    {
      float z;
      float r;
      float w;
    };

    reco::ClusterCompatibility getContainedHits(const std::vector<VertexHit> &hits, double z0) const;

};

ClusterCompatibilityProducer::ClusterCompatibilityProducer(const edm::ParameterSet& config):
  inputTag_     (config.getParameter<edm::InputTag>("inputTag")),
  minZ_         (config.getParameter<double>("minZ")),
  maxZ_         (config.getParameter<double>("maxZ")),
  zStep_        (config.getParameter<double>("zStep"))
{
  inputToken_ = consumes<SiPixelRecHitCollection>(inputTag_);
  LogDebug("") << "Using the " << inputTag_ << " input collection";
  produces<reco::ClusterCompatibilityCollection>();
}

ClusterCompatibilityProducer::~ClusterCompatibilityProducer() {}

void
ClusterCompatibilityProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  std::auto_ptr<reco::ClusterCompatibilityCollection> creco(new reco::ClusterCompatibilityCollection());

  // get hold of products from Event
  edm::Handle<SiPixelRecHitCollection> hRecHits;
  iEvent.getByToken(inputToken_, hRecHits);

  // get tracker geometry
  if (hRecHits.isValid()) {
    edm::ESHandle<TrackerGeometry> trackerHandle;
    iSetup.get<TrackerDigiGeometryRecord>().get(trackerHandle);
    const TrackerGeometry *tgeo = trackerHandle.product();
    const SiPixelRecHitCollection *hits = hRecHits.product();

    // loop over pixel rechits
    int nPxlHits=0;
    std::vector<VertexHit> vhits;
    for(SiPixelRecHitCollection::DataContainer::const_iterator hit = hits->data().begin(),
          end = hits->data().end(); hit != end; ++hit) {
      if (!hit->isValid())
        continue;
      ++nPxlHits;
      DetId id(hit->geographicalId());
      if(id.subdetId() != int(PixelSubdetector::PixelBarrel))
        continue;
      const PixelGeomDetUnit *pgdu = static_cast<const PixelGeomDetUnit*>(tgeo->idToDet(id));
      if (1) {
        const PixelTopology *pixTopo = &(pgdu->specificTopology());
        std::vector<SiPixelCluster::Pixel> pixels(hit->cluster()->pixels());
        bool pixelOnEdge = false;
        for(std::vector<SiPixelCluster::Pixel>::const_iterator pixel = pixels.begin();
            pixel != pixels.end(); ++pixel) {
          int pixelX = pixel->x;
          int pixelY = pixel->y;
          if(pixTopo->isItEdgePixelInX(pixelX) || pixTopo->isItEdgePixelInY(pixelY)) {
            pixelOnEdge = true;
            break;
          }
        }
        if (pixelOnEdge)
          continue;
      }

      LocalPoint lpos = LocalPoint(hit->localPosition().x(),
                                   hit->localPosition().y(),
                                   hit->localPosition().z());
      GlobalPoint gpos = pgdu->toGlobal(lpos);
      VertexHit vh;
      vh.z = gpos.z();
      vh.r = gpos.perp();
      vh.w = hit->cluster()->sizeY();
      vhits.push_back(vh);
    }
 
    // get CompatibleHits for each z-position
    // estimate best z-position from cluster lengths
    double zest = 0.0;
    reco::ClusterCompatibility chits;
    int nhits = 0, nhits_max = 0;
    double chi = 0.0, chi_max = 1e+9;
    for(double z0 = minZ_; z0 <= maxZ_; z0 += zStep_) {
      chits = getContainedHits(vhits, z0);
      creco->push_back(chits); 
      nhits = chits.nHit();
      chi = chits.chi();
      if(nhits == 0)
        continue;
      if(nhits > nhits_max) {
        chi_max = 1e+9;
        nhits_max = nhits;
      }
      if(nhits >= nhits_max && chi < chi_max) {
        chi_max = chi;
        zest = z0;
      }
    }

    // make sure to also store best z-position +/- 10 cm
    // if this is out of range
    double zminus = zest - 10., zplus = zest + 10.;
    if( zminus < minZ_ ) creco->push_back(getContainedHits(vhits, zminus));
    if( zplus > maxZ_ ) creco->push_back(getContainedHits(vhits, zplus));

  }
  iEvent.put(creco);

}


reco::ClusterCompatibility ClusterCompatibilityProducer::getContainedHits(const std::vector<VertexHit> &hits, double z0) const
{
  // Calculate number of hits contained in v-shaped window in cluster y-width vs. z-position.
  int n = 0;
  double chi = 0.;

  for(std::vector<VertexHit>::const_iterator hit = hits.begin(); hit!= hits.end(); hit++) {
    double p = 2 * fabs(hit->z - z0)/hit->r + 0.5; // FIXME <- this comment from the HLT filter, need
    if(fabs(p - hit->w) <= 1.) {                   //          to understand what it means
      chi += fabs(p - hit->w);
      n++;
    }
  }
  return reco::ClusterCompatibility(z0, n, chi);
}

// ------------ method called once each job just before starting event loop  ------------
void 
ClusterCompatibilityProducer::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
ClusterCompatibilityProducer::endJob()
{
}


//define this as a plug-in
DEFINE_FWK_MODULE(ClusterCompatibilityProducer);
