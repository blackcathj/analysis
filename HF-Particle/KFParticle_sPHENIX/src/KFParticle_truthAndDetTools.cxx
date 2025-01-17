#include <g4main/PHG4TruthInfoContainer.h>
#include <trackbase/TrkrDefs.h>
#include <mvtx/MvtxDefs.h>
#include <intt/InttDefs.h>
#include <tpc/TpcDefs.h>
#include <KFParticle_truthAndDetTools.h>
#include <phool/getClass.h>
#include <TTree.h>
#include "KFParticle.h"

std::map<std::string, int> Use = 
{
  { "MVTX",  1 },
  { "INTT",  1 },
  { "TPC",   1 },
  { "EMCAL", 0 },
  { "OHCAL", 0 },
  { "IHCAL", 0 }
};

KFParticle_truthAndDetTools::KFParticle_truthAndDetTools():
    m_svtx_evalstack(nullptr)
{} //Constructor

KFParticle_truthAndDetTools::~KFParticle_truthAndDetTools(){} //Destructor

SvtxTrack* KFParticle_truthAndDetTools::getTrack( unsigned int track_id, SvtxTrackMap *trackmap )
{
    SvtxTrack* matched_track = NULL;
    for ( SvtxTrackMap::Iter iter = trackmap->begin(); iter != trackmap->end(); ++iter )
    {
        if (iter->first == track_id) matched_track = iter->second; 
    }
    return matched_track;
}

void KFParticle_truthAndDetTools::initializeTruthBranches( TTree *m_tree, int daughter_id )
{
  std::string daughter_number = "track_" + std::to_string(daughter_id + 1);

  m_tree->Branch( TString(daughter_number) + "_true_vertex_x", &m_true_daughter_vertex_x[daughter_id], TString(daughter_number) + "_true_vertex_x/F" );
  m_tree->Branch( TString(daughter_number) + "_true_vertex_y", &m_true_daughter_vertex_y[daughter_id], TString(daughter_number) + "_true_vertex_y/F" );
  m_tree->Branch( TString(daughter_number) + "_true_vertex_z", &m_true_daughter_vertex_z[daughter_id], TString(daughter_number) + "_true_vertex_z/F" );
  m_tree->Branch( TString(daughter_number) + "_true_px", &m_true_daughter_px[daughter_id], TString(daughter_number) + "_true_px/F" );
  m_tree->Branch( TString(daughter_number) + "_true_py", &m_true_daughter_py[daughter_id], TString(daughter_number) + "_true_py/F" );
  m_tree->Branch( TString(daughter_number) + "_true_pz", &m_true_daughter_pz[daughter_id], TString(daughter_number) + "_true_pz/F" );
  m_tree->Branch( TString(daughter_number) + "_true_p",  &m_true_daughter_p [daughter_id], TString(daughter_number) + "_true_p/F" );
  m_tree->Branch( TString(daughter_number) + "_true_pt", &m_true_daughter_pt[daughter_id], TString(daughter_number) + "_true_pt/F" );
  m_tree->Branch( TString(daughter_number) + "_true_ID", &m_true_daughter_id[daughter_id], TString(daughter_number) + "_true_ID/I" );
}

void  KFParticle_truthAndDetTools::fillTruthBranch( PHCompositeNode *topNode, TTree *m_tree, KFParticle daughter, int daughter_id )
{
    float true_px, true_py, true_pz, true_p, true_pt;

    if (!m_svtx_evalstack)
    {
      m_svtx_evalstack = new SvtxEvalStack(topNode);
      trackeval = m_svtx_evalstack->get_track_eval();
      clustereval = m_svtx_evalstack->get_cluster_eval();
      trutheval = m_svtx_evalstack->get_truth_eval();
      vertexeval = m_svtx_evalstack->get_vertex_eval();
   }
    m_svtx_evalstack->next_event(topNode);

    dst_trackmap = findNode::getClass<SvtxTrackMap>( topNode, "SvtxTrackMap" );
    track = getTrack( daughter.Id(), dst_trackmap );

    TrkrDefs::cluskey clusKey = *track->begin_cluster_keys();
    g4particle = clustereval->max_truth_particle_by_cluster_energy( clusKey );

    true_px = (Float_t) g4particle->get_px();
    true_py = (Float_t) g4particle->get_py();
    true_pz = (Float_t) g4particle->get_pz();
    true_p  = std::sqrt( std::pow( true_px, 2) + std::pow( true_py, 2) + std::pow( true_pz, 2) );
    true_pt = std::sqrt( std::pow( true_px, 2) + std::pow( true_py, 2) );

    m_true_daughter_px[ daughter_id ] = true_px;
    m_true_daughter_py[ daughter_id ] = true_py;
    m_true_daughter_pz[ daughter_id ] = true_pz;
    m_true_daughter_p [ daughter_id ] = true_p;
    m_true_daughter_pt[ daughter_id ] = true_pt;
    m_true_daughter_id[ daughter_id ] = g4particle->get_pid();

    g4vertex_point = trutheval->get_vertex( g4particle );
    m_true_daughter_vertex_x[ daughter_id ] = g4vertex_point->get_x();
    m_true_daughter_vertex_y[ daughter_id ] = g4vertex_point->get_y();
    m_true_daughter_vertex_z[ daughter_id ] = g4vertex_point->get_z();
}

void KFParticle_truthAndDetTools::initializeDetectorBranches( TTree *m_tree, int daughter_id )
{
    std::string daughter_number = "track_" + std::to_string(daughter_id + 1);

    m_tree->Branch( TString(daughter_number) + "_local_x", &detector_local_x[ daughter_id ] );
    m_tree->Branch( TString(daughter_number) + "_local_y", &detector_local_y[ daughter_id ] );
    m_tree->Branch( TString(daughter_number) + "_local_z", &detector_local_z[ daughter_id ] );
    m_tree->Branch( TString(daughter_number) + "_layer",   &detector_layer  [ daughter_id ] );

    for ( auto const& subdetector : Use )
    {
      if ( subdetector.second ) initializeSubDetectorBranches( m_tree, subdetector.first, daughter_id );
    }
}


void KFParticle_truthAndDetTools::initializeSubDetectorBranches( TTree *m_tree, std::string detectorName, int daughter_id )
{
    std::string daughter_number = "track_" + std::to_string(daughter_id + 1);

    if (detectorName == "MVTX" ) m_tree->Branch( TString(daughter_number) + "_" + TString(detectorName) + "_staveID",     &mvtx_staveID[ daughter_id ] );
    if (detectorName == "MVTX" ) m_tree->Branch( TString(daughter_number) + "_" + TString(detectorName) + "_chipID",      &mvtx_chipID[ daughter_id ] );
    if (detectorName == "INTT" ) m_tree->Branch( TString(daughter_number) + "_" + TString(detectorName) + "_ladderZID",   &intt_ladderZID[ daughter_id ] );
    if (detectorName == "INTT" ) m_tree->Branch( TString(daughter_number) + "_" + TString(detectorName) + "_ladderPhiID", &intt_ladderPhiID[ daughter_id ] );
    if (detectorName == "TPC" )  m_tree->Branch( TString(daughter_number) + "_" + TString(detectorName) + "_sectorID",    &tpc_sectorID[ daughter_id ] );
    if (detectorName == "TPC" )  m_tree->Branch( TString(daughter_number) + "_" + TString(detectorName) + "_side",        &tpc_side[ daughter_id ] );
}

void KFParticle_truthAndDetTools::fillDetectorBranch( PHCompositeNode *topNode, TTree *m_tree, KFParticle daughter, int daughter_id )
{
    dst_clustermap = findNode::getClass<TrkrClusterContainer>( topNode, "TRKR_CLUSTER");
    dst_trackmap = findNode::getClass<SvtxTrackMap>( topNode, "SvtxTrackMap" );
    track = getTrack( daughter.Id(), dst_trackmap );

    for (SvtxTrack::ConstClusterKeyIter iter = track->begin_cluster_keys(); iter != track->end_cluster_keys(); ++iter)
    {
       TrkrDefs::cluskey clusKey = *iter;
       TrkrCluster *cluster = dst_clustermap->findCluster(clusKey);
       const unsigned int trkrId = TrkrDefs::getTrkrId(clusKey);

       detector_local_x[ daughter_id ].push_back( cluster->getX() );
       detector_local_y[ daughter_id ].push_back( cluster->getY() );
       detector_local_z[ daughter_id ].push_back( cluster->getZ() );
       detector_layer[ daughter_id ].push_back( TrkrDefs::getLayer(clusKey) );
       unsigned int staveId, chipId, ladderZId, ladderPhiId, sectorId, side;     
       staveId = chipId = ladderZId = ladderPhiId = sectorId = side = -99;

      if ( Use["MVTX"] && trkrId == TrkrDefs::mvtxId )
      {
        staveId = MvtxDefs::getStaveId(clusKey);
        chipId = MvtxDefs::getChipId(clusKey);
      } 
      else if ( Use["INTT"] && trkrId == TrkrDefs::inttId )
      {
        ladderZId = InttDefs::getLadderZId(clusKey);
        ladderPhiId = InttDefs::getLadderPhiId(clusKey);
      }
      else if ( Use["TPC"] && trkrId == TrkrDefs::tpcId )
      {
        sectorId = TpcDefs::getSectorId(clusKey);
        side = TpcDefs::getSide(clusKey);
      }
      
      mvtx_staveID[ daughter_id ].push_back( staveId );
      mvtx_chipID[ daughter_id ].push_back( chipId );
      intt_ladderZID[ daughter_id ].push_back( ladderZId );
      intt_ladderPhiID[ daughter_id ].push_back( ladderPhiId );
      tpc_sectorID[ daughter_id ].push_back( sectorId );
      tpc_side[ daughter_id ].push_back( side );
   }
}
