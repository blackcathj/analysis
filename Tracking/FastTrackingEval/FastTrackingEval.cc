/*!
 *  \file		FastTrackingEval.cc
 *  \brief		Evaluation module for PHG4TrackFastSim output
 *  \details	input: PHG4TruthInfoContainer, SvtxTrackMap with SvtxTrack_FastSim inside
 *  \author		Haiwang Yu <yuhw@nmsu.edu>
 */

#include "FastTrackingEval.h"

#include <phool/phool.h>
#include <phool/getClass.h>
#include <fun4all/Fun4AllReturnCodes.h>
#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4TruthInfoContainer.h>
#include <g4main/PHG4Particle.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4VtxPoint.h>
#include <fun4all/PHTFileServer.h>
#include <fun4all/Fun4AllServer.h>

#include <g4hough/SvtxVertexMap.h>
#include <g4hough/SvtxVertex.h>
#include <g4hough/SvtxTrackMap.h>
#include <g4hough/SvtxTrack.h>
#include <g4hough/SvtxTrack_FastSim.h>
#include <g4hough/SvtxClusterMap.h>
#include <g4hough/SvtxCluster.h>
#include <g4hough/SvtxHitMap.h>
#include <g4hough/SvtxHit.h>

#include <g4eval/SvtxEvalStack.h>
#include <g4eval/SvtxTrackEval.h>
#include <g4eval/SvtxClusterEval.h>
#include <g4eval/SvtxTruthEval.h>
#include <g4eval/SvtxVertexEval.h>
#include <g4eval/SvtxHitEval.h>

#include <TTree.h>

#include <iostream>

#define LogDebug(exp)		std::cout<<"DEBUG: "	<<__FILE__<<": "<<__LINE__<<": "<< exp <<"\n"
#define LogError(exp)		std::cout<<"ERROR: "	<<__FILE__<<": "<<__LINE__<<": "<< exp <<"\n"
#define LogWarning(exp)	std::cout<<"WARNING: "	<<__FILE__<<": "<<__LINE__<<": "<< exp <<"\n"

using namespace std;

//----------------------------------------------------------------------------//
//-- Constructor:
//--  simple initialization
//----------------------------------------------------------------------------//
FastTrackingEval::FastTrackingEval(const string &name) :
		SubsysReco(name), _flags(NONE), _eval_tree_tracks( NULL) {
	//initialize
	_event = 0;
	_outfile_name = "FastTrackingEval.root";
}

//----------------------------------------------------------------------------//
//-- Init():
//--   Intialize all histograms, trees, and ntuples
//----------------------------------------------------------------------------//
int FastTrackingEval::Init(PHCompositeNode *topNode) {
	cout << PHWHERE << " Openning file " << _outfile_name << endl;
	PHTFileServer::get().open(_outfile_name, "RECREATE");

	// create TTree
	_eval_tree_tracks = new TTree("tracks", "FastSim Eval => tracks");
	_eval_tree_tracks->Branch("event", &event, "event/I");
	_eval_tree_tracks->Branch("gtrackID", &gtrackID, "gtrackID/I");
	_eval_tree_tracks->Branch("gflavor", &gflavor, "gflavor/I");
	_eval_tree_tracks->Branch("gpx", &gpx, "gpx/F");
	_eval_tree_tracks->Branch("gpy", &gpy, "gpy/F");
	_eval_tree_tracks->Branch("gpz", &gpz, "gpz/F");
	_eval_tree_tracks->Branch("gvx", &gvx, "gvx/F");
	_eval_tree_tracks->Branch("gvy", &gvy, "gvy/F");
	_eval_tree_tracks->Branch("gvz", &gvz, "gvz/F");
	_eval_tree_tracks->Branch("trackID", &trackID, "trackID/I");
	_eval_tree_tracks->Branch("charge", &charge, "charge/I");
	_eval_tree_tracks->Branch("nhits", &nhits, "nhits/I");
	_eval_tree_tracks->Branch("px", &px, "px/F");
	_eval_tree_tracks->Branch("py", &py, "py/F");
	_eval_tree_tracks->Branch("pz", &pz, "pz/F");
	_eval_tree_tracks->Branch("dca2d", &dca2d, "dca2d/F");

	return Fun4AllReturnCodes::EVENT_OK;
}

//----------------------------------------------------------------------------//
//-- process_event():
//--   Call user instructions for every event.
//--   This function contains the analysis structure.
//----------------------------------------------------------------------------//
int FastTrackingEval::process_event(PHCompositeNode *topNode) {
	_event++;
	if (_event % 1000 == 0)
		cout << PHWHERE << "Events processed: " << _event << endl;

	GetNodes(topNode);

	fill_tree(topNode);

	return Fun4AllReturnCodes::EVENT_OK;
}

//----------------------------------------------------------------------------//
//-- End():
//--   End method, wrap everything up
//----------------------------------------------------------------------------//
int FastTrackingEval::End(PHCompositeNode *topNode) {

	PHTFileServer::get().cd(_outfile_name);

	_eval_tree_tracks->Write();

	//PHTFileServer::get().close();

	return Fun4AllReturnCodes::EVENT_OK;
}

//----------------------------------------------------------------------------//
//-- fill_tree():
//--   Fill the trees with truth, track fit, and cluster information
//----------------------------------------------------------------------------//
void FastTrackingEval::fill_tree(PHCompositeNode *topNode) {

	// Make sure to reset all the TTree variables before trying to set them.
	reset_variables();

	event = _event;

	if (!_truth_container) {
		LogError("_truth_container not found!");
		return;
	}

	if (!_trackmap) {
		LogError("_trackmap not found!");
		return;
	}

	PHG4TruthInfoContainer::ConstRange range =
			_truth_container->GetPrimaryParticleRange();
	for (PHG4TruthInfoContainer::ConstIterator truth_itr = range.first;
			truth_itr != range.second; ++truth_itr) {

		PHG4Particle* g4particle = truth_itr->second;
		if(!g4particle) {
			LogDebug("");
			continue;
		}

		SvtxTrack_FastSim* track = NULL;

		for (SvtxTrackMap::ConstIter track_itr = _trackmap->begin();
				track_itr != _trackmap->end(); track_itr++) {

			SvtxTrack_FastSim* temp = dynamic_cast<SvtxTrack_FastSim*>(track_itr->second);

			if ((temp->get_truth_track_id() - g4particle->get_track_id()) == 0) {
				track = temp;
			}
		}

		gtrackID = g4particle->get_track_id();
		gflavor = g4particle->get_pid();

		gpx = g4particle->get_px();
		gpy = g4particle->get_py();
		gpz = g4particle->get_pz();


		if (track) {
			trackID = track->get_id();
			charge = track->get_charge();
			nhits = track->size_clusters();

			px = track->get_px();
			py = track->get_py();
			pz = track->get_pz();
			dca2d = track->get_dca2d();
		}

		_eval_tree_tracks->Fill();
	}

	return;

}

//----------------------------------------------------------------------------//
//-- reset_variables():
//--   Reset all the tree variables to their default values.
//--   Needs to be called at the start of every event
//----------------------------------------------------------------------------//
void FastTrackingEval::reset_variables() {
	event = -9999;

	//-- truth
	gtrackID = -9999;
	gflavor = -9999;
	gpx = -9999;
	gpy = -9999;
	gpz = -9999;
	gvx = -9999;
	gvy = -9999;
	gvz = -9999;

	//-- reco
	trackID = -9999;
	charge = -9999;
	nhits = -9999;
	px = -9999;
	py = -9999;
	pz = -9999;
	dca2d = -9999;
}

//----------------------------------------------------------------------------//
//-- GetNodes():
//--   Get all the all the required nodes off the node tree
//----------------------------------------------------------------------------//
int FastTrackingEval::GetNodes(PHCompositeNode * topNode) {
	//DST objects
	//Truth container
	_truth_container = findNode::getClass<PHG4TruthInfoContainer>(topNode,
			"G4TruthInfo");
	if (!_truth_container && _event < 2) {
		cout << PHWHERE << " PHG4TruthInfoContainer node not found on node tree"
				<< endl;
		return Fun4AllReturnCodes::ABORTEVENT;
	}

	_trackmap = findNode::getClass<SvtxTrackMap>(topNode,
			"SvtxTrackMap");
	if (!_trackmap && _event < 2) {
		cout << PHWHERE << "SvtxTrackMap node not found on node tree"
				<< endl;
		return Fun4AllReturnCodes::ABORTEVENT;
	}

	return Fun4AllReturnCodes::EVENT_OK;
}

