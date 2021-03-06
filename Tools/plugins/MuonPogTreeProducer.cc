//////////////////////////////////////
// Ntuplizer that fills muon_pog trees
//////////////////////////////////////

#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/Event.h" 
#include "FWCore/Framework/interface/EventSetup.h"

#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/PatCandidates/interface/Conversion.h"
#include "DataFormats/RecoCandidate/interface/IsoDeposit.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"

#include "DataFormats/Common/interface/View.h"

#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"

#include "DataFormats/METReco/interface/CaloMET.h"
#include "DataFormats/METReco/interface/CaloMETFwd.h"
#include "DataFormats/METReco/interface/PFMET.h"
#include "DataFormats/METReco/interface/PFMETFwd.h"

#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h" 

#include "DataFormats/GeometryVector/interface/VectorUtil.h"
#include "CommonTools/CandUtils/interface/AddFourMomenta.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"

#include "MuonPOG/Tools/src/MuonPogTree.h"
#include "TTree.h"

#include <algorithm>
#include <iostream>

class MuonPogTreeProducer : public edm::EDAnalyzer 
{
public:

  MuonPogTreeProducer(const edm::ParameterSet &);
  
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
  virtual void beginRun(const edm::Run&, const edm::EventSetup&);
  virtual void beginJob();
  virtual void endJob();
  
private:
  
  void fillGenInfo(const edm::Handle<std::vector<PileupSummaryInfo> > &,
		   const  edm::Handle<GenEventInfoProduct> &);

  void fillGenParticles(const edm::Handle<reco::GenParticleCollection> &);

  void fillHlt(const edm::Handle<edm::TriggerResults> &, 
	       const edm::Handle<trigger::TriggerEvent> &,
	       const edm::TriggerNames &);
  
  void fillPV(const edm::Handle<std::vector<reco::Vertex> > &);
  
  void fillMuons(const edm::Handle<reco::MuonCollection> &,
		 const edm::Handle<std::vector<reco::Vertex> > &,
		 const edm::Handle<reco::BeamSpot> &);
  
  edm::InputTag trigResultsTag_;
  edm::InputTag trigSummaryTag_;

  edm::InputTag muonTag_;
  edm::InputTag primaryVertexTag_;
  edm::InputTag beamSpotTag_;

  edm::InputTag pfMetTag_;
  edm::InputTag pfChMetTag_;
  edm::InputTag caloMetTag_;

  edm::InputTag genTag_;
  edm::InputTag pileUpInfoTag_;
  edm::InputTag genInfoTag_;

  muon_pog::Event event_;
  muon_pog::EventId eventId_;
  std::map<std::string,TTree*> tree_;
  
};


MuonPogTreeProducer::MuonPogTreeProducer( const edm::ParameterSet & cfg ) :
  // Input collections
  trigResultsTag_(cfg.getUntrackedParameter<edm::InputTag>("TrigResultsTag", edm::InputTag("TriggerResults::HLT"))),
  trigSummaryTag_(cfg.getUntrackedParameter<edm::InputTag>("TrigSummaryTag", edm::InputTag("hltTriggerSummaryAOD::HLT"))),

  muonTag_(cfg.getUntrackedParameter<edm::InputTag>("MuonTag", edm::InputTag("muons"))),
  primaryVertexTag_(cfg.getUntrackedParameter<edm::InputTag>("PrimaryVertexTag", edm::InputTag("offlinePrimaryVertices"))),
  beamSpotTag_(cfg.getUntrackedParameter<edm::InputTag>("BeamSpotTag", edm::InputTag("offlineBeamSpot"))),

  pfMetTag_(cfg.getUntrackedParameter<edm::InputTag>("PFMetTag", edm::InputTag("pfMet"))), 
  pfChMetTag_(cfg.getUntrackedParameter<edm::InputTag>("PFChMetTag", edm::InputTag("pfChMet"))), 
  caloMetTag_(cfg.getUntrackedParameter<edm::InputTag>("CaloMetTag", edm::InputTag("caloMet"))), 

  genTag_(cfg.getUntrackedParameter<edm::InputTag>("GenTag", edm::InputTag("prunedGenParticles"))),
  pileUpInfoTag_(cfg.getUntrackedParameter<edm::InputTag>("PileUpInfoTag", edm::InputTag("pileupInfo"))),
  genInfoTag_(cfg.getUntrackedParameter<edm::InputTag>("GenInfoTag", edm::InputTag("generator")))
  
{

}


void MuonPogTreeProducer::beginJob() 
{
  
  edm::Service<TFileService> fs;
  tree_["muPogTree"] = fs->make<TTree>("MUONPOGTREE","Muon POG Tree");

  int splitBranches = 2;
  tree_["muPogTree"]->Branch("event",&event_,64000,splitBranches);
  tree_["muPogTree"]->Branch("eventId",&eventId_,64000,splitBranches);

}


void MuonPogTreeProducer::beginRun(const edm::Run & run, const edm::EventSetup & config )
{
  
}


void MuonPogTreeProducer::endJob() 
{

}


void MuonPogTreeProducer::analyze (const edm::Event & ev, const edm::EventSetup &)
{

  // Clearing branch variables
  // and setting default values
  event_.hlt.triggers.clear();
  event_.hlt.objects.clear();

  event_.genParticles.clear();
  event_.genInfos.clear();
  event_.muons.clear();
  
  event_.mets.pfMet   = -999; 
  event_.mets.pfChMet = -999; 
  event_.mets.caloMet = -999; 

  for (unsigned int ix=0; ix<3; ++ix) {
    event_.primaryVertex[ix] = 0.;
    for (unsigned int iy=0; iy<3; ++iy) {
      event_.cov_primaryVertex[ix][iy] = 0.;
    }
  }
  event_.nVtx = -1;


  // Fill general information
  // run, luminosity block, event
  event_.runNumber = ev.id().run();
  event_.luminosityBlockNumber = ev.id().luminosityBlock();
  event_.eventNumber = ev.id().event();

  eventId_.runNumber = ev.id().run();
  eventId_.luminosityBlockNumber = ev.id().luminosityBlock();
  eventId_.eventNumber = ev.id().event();

  // Fill GEN pile up information
  if (!ev.isRealData()) 
    {
      if (pileUpInfoTag_.label() != "none" &&
	  genInfoTag_.label() != "none") 
	{
	  edm::Handle<std::vector<PileupSummaryInfo> > puInfo;
	  edm::Handle<GenEventInfoProduct> genInfo;

	  if (ev.getByLabel(pileUpInfoTag_, puInfo) &&
	      ev.getByLabel(genInfoTag_, genInfo) ) 
	    fillGenInfo(puInfo,genInfo);
	  else 
	    edm::LogError("") << "[MuonPogTreeProducer]: Pile-Up Info collection does not exist !!!";
	}      
    }
  

  // Fill GEN particles information
  if (!ev.isRealData()) 
    {
      if (genTag_.label() != "none" ) 
	{ 
	  edm::Handle<reco::GenParticleCollection> genParticles;
	  if (ev.getByLabel(genTag_, genParticles)) 
	    fillGenParticles(genParticles);
	  else 
	    edm::LogError("") << ">>> GEN collection does not exist !!!";
	}
    }
  

  // Fill trigger information
  if (trigResultsTag_.label() != "none" &&
      trigSummaryTag_.label() != "none") 
    {
      
      edm::Handle<edm::TriggerResults> triggerResults;
      edm::Handle<trigger::TriggerEvent> triggerEvent;
      
      if (ev.getByLabel(trigResultsTag_, triggerResults) &&
	  ev.getByLabel(trigSummaryTag_, triggerEvent)) 
	fillHlt(triggerResults, triggerEvent,ev.triggerNames(*triggerResults));
      else 
	edm::LogError("") << "[MuonPogTreeProducer]: Trigger collections do not exist !!!";
    }
  
  
  // Fill vertex information
  edm::Handle<std::vector<reco::Vertex> > vertexes;

  if(primaryVertexTag_.label() != "none") 
    {
      if (ev.getByLabel(primaryVertexTag_, vertexes))
	fillPV(vertexes);
      else 
	edm::LogError("") << "[MuonPogTreeProducer]: Vertex collection does not exist !!!";
    }

  // Get beam spot for muons
  edm::Handle<reco::BeamSpot> beamSpot;
  if (beamSpotTag_.label() != "none" ) 
    { 
      if (!ev.getByLabel(beamSpotTag_, beamSpot)) 
	edm::LogError("") << "[MuonPogTreeProducer]: Beam spot collection not found !!!";
    }

  // Fill (raw) MET information: PF, PF charged, Calo    
  edm::Handle<reco::PFMETCollection> pfMet; 
  if(pfMetTag_.label() != "none") 
    { 
      if (!ev.getByLabel(pfMetTag_, pfMet)) 
	edm::LogError("") << "[MuonPogTreeProducer] PFMet collection does not exist !!!"; 
      else { 
	const reco::PFMET &iPfMet = (*pfMet)[0]; 
	event_.mets.pfMet = iPfMet.et(); 
      } 
    } 

  edm::Handle<reco::PFMETCollection> pfChMet; 
  if(pfChMetTag_.label() != "none") 
    { 
      if (!ev.getByLabel(pfChMetTag_, pfChMet)) 
	edm::LogError("") << "[MuonPogTreeProducer] PFChMet collection does not exist !!!"; 
      else { 
	const reco::PFMET &iPfChMet = (*pfChMet)[0]; 
	event_.mets.pfChMet = iPfChMet.et(); 
      } 
    } 

  edm::Handle<reco::CaloMETCollection> caloMet; 
  if(caloMetTag_.label() != "none") 
    { 
      if (!ev.getByLabel(caloMetTag_, caloMet)) 
	edm::LogError("") << "[MuonPogTreeProducer] CaloMet collection does not exist !!!"; 
      else { 
	const reco::CaloMET &iCaloMet = (*caloMet)[0]; 
	event_.mets.caloMet = iCaloMet.et(); 
      } 
    } 

  // Get muons  
  edm::Handle<reco::MuonCollection> muons;
  if (muonTag_.label() != "none" ) 
    { 
      if (!ev.getByLabel(muonTag_, muons)) 
	edm::LogError("") << "[MuonPogTreeProducer] Muon collection does not exist !!!";
    }
  

  // Fill muon information
  if (muons.isValid() && vertexes.isValid() && beamSpot.isValid()) 
    {
      fillMuons(muons,vertexes,beamSpot);
    }
  
  tree_["muPogTree"]->Fill();
  
}


void MuonPogTreeProducer::fillGenInfo(const edm::Handle<std::vector<PileupSummaryInfo> > & puInfo,
				      const edm::Handle<GenEventInfoProduct> & gen)
{

  muon_pog::GenInfo genInfo;
  
  genInfo.trueNumberOfInteractions     = -1.;
  genInfo.actualNumberOfInteractions   = -1.;
  genInfo.genWeight = gen->weight() ;
	
  std::vector<PileupSummaryInfo>::const_iterator puInfoIt  = puInfo->begin();
  std::vector<PileupSummaryInfo>::const_iterator puInfoEnd = puInfo->end();

  for(; puInfoIt != puInfoEnd; ++puInfoIt) 
    {
      int bx = puInfoIt->getBunchCrossing();
	  
      if(bx == 0) 
	{ 
	  genInfo.trueNumberOfInteractions   = puInfoIt->getTrueNumInteractions();
	  genInfo.actualNumberOfInteractions = puInfoIt->getPU_NumInteractions();
	  continue;
	}
    }
  
  event_.genInfos.push_back(genInfo);
  
}


void MuonPogTreeProducer::fillGenParticles(const edm::Handle<reco::GenParticleCollection> & genParticles)
{
  
  unsigned int gensize = genParticles->size();
  
  // Do not record the initial protons
  for (unsigned int i=0; i<gensize; ++i) 
    {

      const reco::GenParticle& part = genParticles->at(i);
    
      muon_pog::GenParticle gensel;
      gensel.pdgId = part.pdgId();
      gensel.status = part.status();
      gensel.energy = part.energy();
      gensel.pt = part.pt();
      gensel.eta = part.eta();
      gensel.phi = part.phi();
      gensel.vx = part.vx();
      gensel.vy = part.vy();
      gensel.vz = part.vz();

      gensel.mothers.clear();
      unsigned int nMothers = part.numberOfMothers();

      for (unsigned int iMother=0; iMother<nMothers; ++iMother) 
	{
	  gensel.mothers.push_back(part.motherRef(iMother)->pdgId());
	}

      // Protect agains bug in genParticles (missing mother => first proton)
      if (i>=2 && nMothers==0) gensel.mothers.push_back(0);
      
      event_.genParticles.push_back(gensel);
    }
  
}


void MuonPogTreeProducer::fillHlt(const edm::Handle<edm::TriggerResults> & triggerResults, 
				  const edm::Handle<trigger::TriggerEvent> & triggerEvent,
				  const edm::TriggerNames & triggerNames)
{    

  for (unsigned int iTrig=0; iTrig<triggerNames.size(); ++iTrig) 
    {
      
      if (triggerResults->accept(iTrig)) 
	{
	  std::string pathName = triggerNames.triggerName(iTrig);
	  event_.hlt.triggers.push_back(pathName);
	}
    }
      
  const trigger::size_type nFilters(triggerEvent->sizeFilters());

  for (trigger::size_type iFilter=0; iFilter!=nFilters; ++iFilter) 
    {
	
      std::string filterTag = triggerEvent->filterTag(iFilter).encode();

      trigger::Keys objectKeys = triggerEvent->filterKeys(iFilter);
      const trigger::TriggerObjectCollection& triggerObjects(triggerEvent->getObjects());
	
      for (trigger::size_type iKey=0; iKey<objectKeys.size(); ++iKey) 
	{  
	  trigger::size_type objKey = objectKeys.at(iKey);
	  const trigger::TriggerObject& triggerObj(triggerObjects[objKey]);
	  
	  muon_pog::HLTObject hltObj;
	  
	  float trigObjPt = triggerObj.pt();
	  float trigObjEta = triggerObj.eta();
	  float trigObjPhi = triggerObj.phi();
	  
	  hltObj.filterTag = filterTag;

	  hltObj.pt  = trigObjPt;
	  hltObj.eta = trigObjEta;
	  hltObj.phi = trigObjPhi;
	  
	  event_.hlt.objects.push_back(hltObj);
	  
	}       
    }

}


void MuonPogTreeProducer::fillPV(const edm::Handle<std::vector<reco::Vertex> > & vertexes)
{
      
  int nVtx = 0;

  std::vector<reco::Vertex>::const_iterator vertexIt  = vertexes->begin();
  std::vector<reco::Vertex>::const_iterator vertexEnd = vertexes->end();

  for (; vertexIt != vertexEnd; ++vertexIt) 
    {

      const reco::Vertex& vertex = *vertexIt;

      if (!vertex.isValid()) continue;
      ++nVtx;

      if (vertexIt == vertexes->begin()) 
	{
	  event_.primaryVertex[0] = vertex.x();
	  event_.primaryVertex[1] = vertex.y();
	  event_.primaryVertex[2] = vertex.z();

	  for (unsigned int ix=0; ix<3; ++ix) 
	    {
	      for (unsigned int iy=0; iy<3; ++iy) 
		{
		  event_.cov_primaryVertex[ix][iy] = vertex.covariance(ix,iy);
		}
	    }
	}
    }
  
  event_.nVtx = nVtx;
  
}


void MuonPogTreeProducer::fillMuons(const edm::Handle<reco::MuonCollection> & muons,
				    const edm::Handle<std::vector<reco::Vertex> > & vertexes,
				    const edm::Handle<reco::BeamSpot> & beamSpot)
{

  reco::MuonCollection::const_iterator muonIt  = muons->begin();
  reco::MuonCollection::const_iterator muonEnd = muons->end();

  for (; muonIt != muonEnd; ++muonIt) 
    {
      
      const reco::Muon& mu = (*muonIt);

      bool isGlobal      = mu.isGlobalMuon();
      bool isTracker     = mu.isTrackerMuon();
      bool isTrackerArb  = muon::isGoodMuon(mu, muon::TrackerMuonArbitrated); 
      bool isRPC         = mu.isRPCMuon();
      bool isStandAlone  = mu.isStandAloneMuon();
      bool isPF          = mu.isPFMuon();

      bool hasInnerTrack = !mu.innerTrack().isNull();
      bool hasTunePTrack = !mu.tunePMuonBestTrack().isNull();
      
      muon_pog::Muon ntupleMu;
      
      ntupleMu.pt     = mu.pt();
      ntupleMu.eta    = mu.eta();
      ntupleMu.phi    = mu.phi();
      ntupleMu.charge = mu.charge();

      ntupleMu.pt_global     = isGlobal ? mu.globalTrack()->pt()  : -1000.;
      ntupleMu.eta_global    = isGlobal ? mu.globalTrack()->eta() : -1000.;
      ntupleMu.phi_global    = isGlobal ? mu.globalTrack()->phi() : -1000.;
      ntupleMu.charge_global = isGlobal ? mu.globalTrack()->charge() : -1000.;

      ntupleMu.pt_tuneP     = hasTunePTrack ? mu.tunePMuonBestTrack()->pt()  : -1000.;
      ntupleMu.eta_tuneP    = hasTunePTrack ? mu.tunePMuonBestTrack()->eta() : -1000.;
      ntupleMu.phi_tuneP    = hasTunePTrack ? mu.tunePMuonBestTrack()->phi() : -1000.;
      ntupleMu.charge_tuneP = hasTunePTrack ? mu.tunePMuonBestTrack()->charge() : -1000.;

      ntupleMu.pt_tracker     = hasInnerTrack ? mu.innerTrack()->pt()  : -1000.;
      ntupleMu.eta_tracker    = hasInnerTrack ? mu.innerTrack()->eta() : -1000.;
      ntupleMu.phi_tracker    = hasInnerTrack ? mu.innerTrack()->phi() : -1000.;
      ntupleMu.charge_tracker = hasInnerTrack ? mu.innerTrack()->charge() : -1000.;

      reco::MuonPFIsolation iso04 = mu.pfIsolationR04();
      reco::MuonPFIsolation iso03 = mu.pfIsolationR03();

      ntupleMu.chargedHadronIso   = iso04.sumChargedHadronPt;
      ntupleMu.chargedHadronIsoPU = iso04.sumPUPt; 
      ntupleMu.neutralHadronIso   = iso04.sumNeutralHadronEt;
      ntupleMu.photonIso          = iso04.sumPhotonEt;

      ntupleMu.isGlobal     = isGlobal ? 1 : 0;	
      ntupleMu.isTracker    = isTracker ? 1 : 0;	
      ntupleMu.isTrackerArb = isTrackerArb ? 1 : 0;	
      ntupleMu.isRPC        = isRPC ? 1 : 0;
      ntupleMu.isStandAlone = isStandAlone ? 1 : 0;
      ntupleMu.isPF         = isPF ? 1 : 0;

      ntupleMu.nHitsGlobal     = isGlobal     ? mu.globalTrack()->numberOfValidHits() : -999;	
      ntupleMu.nHitsTracker    = isTracker    ? mu.innerTrack()->numberOfValidHits()  : -999;	
      ntupleMu.nHitsStandAlone = isStandAlone ? mu.outerTrack()->numberOfValidHits()  : -999;

      ntupleMu.glbNormChi2              = isGlobal      ? mu.globalTrack()->normalizedChi2() : -999; 
      ntupleMu.trkNormChi2	        = hasInnerTrack ? mu.innerTrack()->normalizedChi2()  : -999; 
      ntupleMu.trkMuonMatchedStations   = isTracker     ? mu.numberOfMatchedStations()       : -999; 
      ntupleMu.glbMuonValidHits	        = isGlobal      ? mu.globalTrack()->hitPattern().numberOfValidMuonHits()       : -999; 
      ntupleMu.trkPixelValidHits	= hasInnerTrack ? mu.innerTrack()->hitPattern().numberOfValidPixelHits()       : -999; 
      ntupleMu.trkPixelLayersWithMeas   = hasInnerTrack ? mu.innerTrack()->hitPattern().pixelLayersWithMeasurement()   : -999; 
      ntupleMu.trkTrackerLayersWithMeas = hasInnerTrack ? mu.innerTrack()->hitPattern().trackerLayersWithMeasurement() : -999; 

      ntupleMu.bestMuPtErr              = mu.muonBestTrack()->ptError(); 

      ntupleMu.trkValidHitFrac = hasInnerTrack           ? mu.innerTrack()->validFraction()       : -999; 
      ntupleMu.trkStaChi2      = isGlobal                ? mu.combinedQuality().chi2LocalPosition : -999; 
      ntupleMu.trkKink         = isGlobal                ? mu.combinedQuality().trkKink           : -999; 
      ntupleMu.muSegmComp      = (isGlobal || isTracker) ? muon::segmentCompatibility(mu)         : -999; 

      ntupleMu.isTrkMuOST               = muon::isGoodMuon(mu, muon::TMOneStationTight) ? 1 : 0; 
      ntupleMu.isTrkHP                  = hasInnerTrack && mu.innerTrack()->quality(reco::TrackBase::highPurity) ? 1 : 0; 

      ntupleMu.dxyBest  = -999; 
      ntupleMu.dzBest   = -999; 
      ntupleMu.dxyInner = -999; 
      ntupleMu.dzInner  = -999; 

      ntupleMu.isoPflow04 = (iso04.sumChargedHadronPt+ std::max(0.,iso04.sumPhotonEt+iso04.sumNeutralHadronEt - 0.5*iso04.sumPUPt)) / mu.pt();
    
      ntupleMu.isoPflow03 = (iso03.sumChargedHadronPt+ std::max(0.,iso03.sumPhotonEt+iso03.sumNeutralHadronEt - 0.5*iso03.sumPUPt)) / mu.pt();

      const reco::Vertex & vertex = vertexes->at(0); // CB for now vertex is always valid, but add a protection	    
      
      double dxybs = isGlobal ? mu.globalTrack()->dxy(beamSpot->position()) :
	hasInnerTrack ? mu.innerTrack()->dxy(beamSpot->position()) : -1000;
      double dzbs  = isGlobal ? mu.globalTrack()->dz(beamSpot->position()) :
	hasInnerTrack ? mu.innerTrack()->dz(beamSpot->position()) : -1000;

      double dxy = -1000.;
      double dz  = -1000.;

      ntupleMu.isSoft    = 0;	  
      ntupleMu.isTight   = 0;	  
      ntupleMu.isHighPt  = 0;
      ntupleMu.isLoose   = muon::isLooseMuon(mu)  ? 1 : 0;	  
      ntupleMu.isMedium  = muon::isMediumMuon(mu) ? 1 : 0;	  

      if (vertexes->size() > 0)
	{
	  const reco::Vertex & vertex = vertexes->at(0);

	  dxy = isGlobal ? mu.globalTrack()->dxy(vertex.position()) :
	    hasInnerTrack ? mu.innerTrack()->dxy(vertex.position()) : -1000;
	  dz = isGlobal ? mu.globalTrack()->dz(vertex.position()) :
	    hasInnerTrack ? mu.innerTrack()->dz(vertex.position()) : -1000;
 
	  ntupleMu.dxyBest  = mu.muonBestTrack()->dxy(vertex.position()); 
	  ntupleMu.dzBest   = mu.muonBestTrack()->dz(vertex.position()); 
	  if(hasInnerTrack) { 
	    ntupleMu.dxyInner = mu.innerTrack()->dxy(vertex.position()); 
	    ntupleMu.dzInner  = mu.innerTrack()->dz(vertex.position()); 
	  } 

	  ntupleMu.isSoft    = muon::isSoftMuon(mu,vertex)   ? 1 : 0;	  
	  ntupleMu.isTight   = muon::isTightMuon(mu,vertex)  ? 1 : 0;	  
	  ntupleMu.isHighPt  = muon::isHighPtMuon(mu,vertex) ? 1 : 0;

	}

      ntupleMu.dxy    = dxy;
      ntupleMu.dz     = dz;
      ntupleMu.edxy   = isGlobal ? mu.globalTrack()->dxyError() : hasInnerTrack ? mu.innerTrack()->dxyError() : -1000;
      ntupleMu.edz    = isGlobal ? mu.globalTrack()->dzError()  : hasInnerTrack ? mu.innerTrack()->dzError() : -1000;

      ntupleMu.dxybs  = dxybs;
      ntupleMu.dzbs   = dzbs;

      if(mu.isTimeValid()) { 
	ntupleMu.muonTimeDof = mu.time().nDof; 
	ntupleMu.muonTime    = mu.time().timeAtIpInOut; 
	ntupleMu.muonTimeErr = mu.time().timeAtIpInOutErr; 
      } 
      else { 
	ntupleMu.muonTimeDof = -999; 
	ntupleMu.muonTime    = -999; 
	ntupleMu.muonTimeErr = -999; 
      } 

      event_.muons.push_back(ntupleMu);

    }

}


#include "FWCore/Framework/interface/MakerMacros.h"

DEFINE_FWK_MODULE(MuonPogTreeProducer);
