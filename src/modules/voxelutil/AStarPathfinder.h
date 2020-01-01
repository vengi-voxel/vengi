/**
 * @file
 */

#pragma once

#include "AStarPathfinderImpl.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/GLM.h"

#include <functional>
#include <list>

namespace voxel {

/**
 * This function provides the default method for checking whether a given voxel
 * is valid for the path computed by the AStarPathfinder.
 */
template<typename VolumeType>
bool aStarDefaultVoxelValidator(const VolumeType* volData, const glm::ivec3& v3dPos);

/**
 * @brief Provides a configuration for the AStarPathfinder.
 *
 * This structure stores the AStarPathfinder%s configuration options, because this
 * is simpler than providing a large number of get/set properties within the
 * AStarPathfinder itself. In order to create an instance of this structure you
 * must provide at least a volume, a start and end point, and a list to store
 * the result. All the other option have sensible default values which can
 * optionally be changed for more precise control over the pathfinder's behaviour.
 *
 * @sa AStarPathfinder
 */
template<typename VolumeType>
struct AStarPathfinderParams {
public:
	AStarPathfinderParams(VolumeType* volData, const glm::ivec3& v3dStart, const glm::ivec3& v3dEnd, std::list<glm::ivec3>* listResult, float fHBias = 1.0,
			uint32_t uMaxNoOfNodes = 10000, Connectivity requiredConnectivity = TwentySixConnected,
			std::function<bool(const VolumeType*, const glm::ivec3&)> funcIsVoxelValidForPath = &aStarDefaultVoxelValidator, std::function<void(float)> funcProgressCallback =
					nullptr) :
			volume(volData), start(v3dStart), end(v3dEnd), result(listResult), connectivity(requiredConnectivity), hBias(fHBias), maxNumberOfNodes(uMaxNoOfNodes), isVoxelValidForPath(
					funcIsVoxelValidForPath), progressCallback(funcProgressCallback) {
	}

	/// This is the volume through which the AStarPathfinder must find a path.
	VolumeType* volume;

	/// The start point for the pathfinding algorithm.
	glm::ivec3 start;

	/// The end point for the pathfinding algorithm.
	glm::ivec3 end;

	/// The resulting path will be stored as a series of points in
	/// this list. Any existing contents will be cleared.
	std::list<glm::ivec3>* result;

	/// The AStarPathfinder performs its search by examining the neighbours
	/// of each voxel it encounters. This property controls the meaning of
	/// neighbour - e.g. whether two voxels must share a face, edge, or corner.
	Connectivity connectivity;

	/// For each voxel the pathfinder tracks its distance to the start (known as g())
	/// and estimates its distance to the end (known as h()). Increasing or decreasing
	/// h() has an effect on the way the pathfinder behaves. If h() is an underestimate
	/// of the true distance then the pathfinder will act more like a greedy search -
	/// always finding the shortest path but taking longer to do so. If h() is an over
	/// estimate then the pathfinder will behave more like a best-first search - returning
	/// a potentially suboptimal path but finding it more quickly. The hBias is multiplied
	/// by the estimated h() value to control this behaviour.
	float hBias;

	/// Volumes can be pretty huge (millions of voxels) and processing each one of these
	/// can take a long time. In A* terminology each voxel is a node, and this property
	/// controls the maximum number of nodes that will be considered when finding the path,
	/// before giving up
	uint32_t maxNumberOfNodes;

	/// This function is called to determine whether the path can pass though a given voxel. The
	/// default behaviour is specified by aStarDefaultVoxelValidator(), but users can specify their
	/// own criteria if desired. For example, if you always want a path to follow a surface then
	/// you could check to ensure that the voxel above is empty and the voxel below is solid.
	///
	/// @sa aStarDefaultVoxelValidator
	std::function<bool(const VolumeType*, const glm::ivec3&)> isVoxelValidForPath;

	/// This function is called by the AStarPathfinder to report on its progress in getting to
	/// the goal. The progress is reported by computing the distance from the closest node found
	/// so far to the end node, and comparing this with the distance from the start node to the
	/// end node. This progress value is guaranteed to never decrease, but it may stop increasing
	/// for short periods of time. It may even stop increasing altogether if a path cannot be found.
	std::function<void(float)> progressCallback;
};

/**
 * @brief The AStarPathfinder compute a path from one point in the volume to another.
 *
 * A* is a well known pathfinding algorithm commonly used in computer games. It
 * takes as input a pair of points in the world, and works out a path between
 * them which avoids obstacles and adheres to other user defined criteria. The
 * resulting path is usually the shortest possible, but a less optimal path can
 * be exchanged for reduced computation time.
 *
 * For an excellent overview of the A* algorithm please see Amit Patel's Game
 * Programming page here: http://theory.stanford.edu/~amitp/GameProgramming/
 * Much of this class is based on the principles described in those pages.
 *
 * Usage of this class if very straightforward. You create an instance of it
 * by passing an instance of the AStarPathfinderParams structure to the constructor.
 * The details of the AStarPathfinderParams and the options it provides are described
 * in the documentation for that class.
 *
 * Next you call the execute() function and wait for it to return. If a path is
 * found then this is stored in the list which was set as the 'result' field of
 * the AStarPathfinderParams.
 *
 * @sa AStarPathfinderParams
 */
template<typename VolumeType>
class AStarPathfinder {
public:
	AStarPathfinder(const AStarPathfinderParams<VolumeType>& params);

	bool execute();

private:
	void processNeighbour(const glm::ivec3& neighbourPos, float neighbourGVal);

	float SixConnectedCost(const glm::ivec3& a, const glm::ivec3& b);
	float EighteenConnectedCost(const glm::ivec3& a, const glm::ivec3& b);
	float TwentySixConnectedCost(const glm::ivec3& a, const glm::ivec3& b);
	float computeH(const glm::ivec3& a, const glm::ivec3& b);
	uint32_t hash(uint32_t a);

	// Node containers
	AllNodesContainer _allNodes;
	OpenNodesContainer _openNodes;
	ClosedNodesContainer _closedNodes;

	// The current node
	AllNodesContainer::iterator _current;

	float _progress = 0.0f;

	AStarPathfinderParams<VolumeType> _params;
};

/**
 * @section Useful constants
 */

const glm::ivec3 arrayPathfinderFaces[6] = {
		glm::ivec3(0, 0, -1),
		glm::ivec3(0, 0, +1),
		glm::ivec3(0, -1, 0),
		glm::ivec3(0, +1, 0),
		glm::ivec3(-1, 0, 0),
		glm::ivec3(+1, 0, 0) };

const glm::ivec3 arrayPathfinderEdges[12] = {
		glm::ivec3(0, -1, -1),
		glm::ivec3(0, -1, +1),
		glm::ivec3(0, +1, -1),
		glm::ivec3(0, +1, +1),
		glm::ivec3(-1, 0, -1),
		glm::ivec3(-1, 0, +1),
		glm::ivec3(+1, 0, -1),
		glm::ivec3(+1, 0, +1),
		glm::ivec3(-1, -1, 0),
		glm::ivec3(-1, +1, 0),
		glm::ivec3(+1, -1, 0),
		glm::ivec3(+1, +1, 0) };

const glm::ivec3 arrayPathfinderCorners[8] = {
		glm::ivec3(-1, -1, -1),
		glm::ivec3(-1, -1, +1),
		glm::ivec3(-1, +1, -1),
		glm::ivec3(-1, +1, +1),
		glm::ivec3(+1, -1, -1),
		glm::ivec3(+1, -1, +1),
		glm::ivec3(+1, +1, -1),
		glm::ivec3(+1, +1, +1) };

/**
 * Using this function, a voxel is considered valid for the path if it is inside the
 * volume.
 * @return true is the voxel is valid for the path
 */
template<typename VolumeType>
bool aStarDefaultVoxelValidator(const VolumeType* volData, const glm::ivec3& v3dPos) {
	return volData->getRegion().containsPoint(v3dPos);
}

/**
 * @section AStarPathfinder Class
 */
template<typename VolumeType>
AStarPathfinder<VolumeType>::AStarPathfinder(const AStarPathfinderParams<VolumeType>& params) :
		_params(params) {
}

template<typename VolumeType>
bool AStarPathfinder<VolumeType>::execute() {
	//Clear any existing nodes
	_allNodes.clear();
	_openNodes.clear();
	_closedNodes.clear();

	//Clear the result
	_params.result->clear();

	//Iterators to start and end node.
	AllNodesContainer::iterator startNode = _allNodes.insert(Node(_params.start.x, _params.start.y, _params.start.z)).first;
	AllNodesContainer::iterator endNode = _allNodes.insert(Node(_params.end.x, _params.end.y, _params.end.z)).first;

	//Regarding the const_cast - normally you should not modify an object which is in an sdt::set.
	//The reason is that objects in a set are stored sorted in a tree so they can be accessed quickly,
	//and changing the object directly can break the sorting. However, in our case we have provided a
	//custom sort operator for the set which we know only uses the position to sort. Hence we can safely
	//modify other properties of the object while it is in the set.
	Node* tempStart = const_cast<Node*>(&(*startNode));
	tempStart->gVal = 0;
	tempStart->hVal = computeH(startNode->position, endNode->position);

	Node* tempEnd = const_cast<Node*>(&(*endNode));
	tempEnd->hVal = 0.0f;

	_openNodes.insert(startNode);

	float fDistStartToEnd = (endNode->position - startNode->position).length();
	_progress = 0.0f;
	if (_params.progressCallback) {
		_params.progressCallback(_progress);
	}

	while (!_openNodes.empty() && _openNodes.getFirst() != endNode) {
		//Move the first node from open to closed.
		_current = _openNodes.getFirst();
		_openNodes.removeFirst();
		_closedNodes.insert(_current);

		//Update the user on our progress
		if (_params.progressCallback) {
			const float fMinProgresIncreament = 0.001f;
			float fDistCurrentToEnd = (endNode->position - _current->position).length();
			float fDistNormalised = fDistCurrentToEnd / fDistStartToEnd;
			float fProgress = 1.0f - fDistNormalised;
			if (fProgress >= _progress + fMinProgresIncreament) {
				_progress = fProgress;
				_params.progressCallback(_progress);
			}
		}

		//The distance from one cell to another connected by face, edge, or corner.
		const float fFaceCost = 1.0f;
		const float fEdgeCost = glm::root_two<float>();
		const float fCornerCost = glm::root_three<float>();

		//Process the neighbours. Note the deliberate lack of 'break'
		//statements, larger connectivities include smaller ones.
		switch (_params.connectivity) {
		case TwentySixConnected:
			processNeighbour(_current->position + arrayPathfinderCorners[0], _current->gVal + fCornerCost);
			processNeighbour(_current->position + arrayPathfinderCorners[1], _current->gVal + fCornerCost);
			processNeighbour(_current->position + arrayPathfinderCorners[2], _current->gVal + fCornerCost);
			processNeighbour(_current->position + arrayPathfinderCorners[3], _current->gVal + fCornerCost);
			processNeighbour(_current->position + arrayPathfinderCorners[4], _current->gVal + fCornerCost);
			processNeighbour(_current->position + arrayPathfinderCorners[5], _current->gVal + fCornerCost);
			processNeighbour(_current->position + arrayPathfinderCorners[6], _current->gVal + fCornerCost);
			processNeighbour(_current->position + arrayPathfinderCorners[7], _current->gVal + fCornerCost);
			/* fallthrough */

		case EighteenConnected:
			processNeighbour(_current->position + arrayPathfinderEdges[0], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[1], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[2], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[3], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[4], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[5], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[6], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[7], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[8], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[9], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[10], _current->gVal + fEdgeCost);
			processNeighbour(_current->position + arrayPathfinderEdges[11], _current->gVal + fEdgeCost);
			/* fallthrough */

		case SixConnected:
			processNeighbour(_current->position + arrayPathfinderFaces[0], _current->gVal + fFaceCost);
			processNeighbour(_current->position + arrayPathfinderFaces[1], _current->gVal + fFaceCost);
			processNeighbour(_current->position + arrayPathfinderFaces[2], _current->gVal + fFaceCost);
			processNeighbour(_current->position + arrayPathfinderFaces[3], _current->gVal + fFaceCost);
			processNeighbour(_current->position + arrayPathfinderFaces[4], _current->gVal + fFaceCost);
			processNeighbour(_current->position + arrayPathfinderFaces[5], _current->gVal + fFaceCost);
			break;
		}

		if (_allNodes.size() > _params.maxNumberOfNodes) {
			//We've reached the specified maximum number
			//of nodes. Just give up on the search.
			break;
		}
	}

	if (_openNodes.empty() || _openNodes.getFirst() != endNode) {
		//In this case we failed to find a valid path.
		return false;
	}
	//Regarding the const_cast - normally you should not modify an object which is in an sdt::set.
	//The reason is that objects in a set are stored sorted in a tree so they can be accessed quickly,
	//and changing the object directly can break the sorting. However, in our case we have provided a
	//custom sort operator for the set which we know only uses the position to sort. Hence we can safely
	//modify other properties of the object while it is in the set.
	Node* n = const_cast<Node*>(&(*endNode));
	while (n != 0) {
		_params.result->push_front(n->position);
		n = n->parent;
	}

	if (_params.progressCallback) {
		_params.progressCallback(1.0f);
	}

	return true;
}

template<typename VolumeType>
void AStarPathfinder<VolumeType>::processNeighbour(const glm::ivec3& neighbourPos, float neighbourGVal) {
	bool bIsVoxelValidForPath = _params.isVoxelValidForPath(_params.volume, neighbourPos);
	if (!bIsVoxelValidForPath) {
		return;
	}

	float cost = neighbourGVal;

	std::pair<AllNodesContainer::iterator, bool> insertResult = _allNodes.insert(Node(neighbourPos.x, neighbourPos.y, neighbourPos.z));
	AllNodesContainer::iterator neighbour = insertResult.first;

	if (insertResult.second) {
		//New node, compute h.
		Node* tempNeighbour = const_cast<Node*>(&(*neighbour));
		tempNeighbour->hVal = computeH(neighbour->position, _params.end);
	}

	OpenNodesContainer::iterator openIter = _openNodes.find(neighbour);
	if (openIter != _openNodes.end()) {
		if (cost < neighbour->gVal) {
			_openNodes.remove(openIter);
			openIter = _openNodes.end();
		}
	}

	//TODO - Nodes could keep track of if they are in open or closed? And a pointer to where they are?
	ClosedNodesContainer::iterator closedIter = _closedNodes.find(neighbour);
	if (closedIter != _closedNodes.end()) {
		if (cost < neighbour->gVal) {
			//Probably shouldn't happen?
			_closedNodes.remove(closedIter);
			closedIter = _closedNodes.end();
		}
	}

	if (openIter == _openNodes.end() && closedIter == _closedNodes.end()) {
		//Regarding the const_cast - normally you should not modify an object which is in an sdt::set.
		//The reason is that objects in a set are stored sorted in a tree so they can be accessed quickly,
		//and changing the object directly can break the sorting. However, in our case we have provided a
		//custom sort operator for the set which we know only uses the position to sort. Hence we can safely
		//modify other properties of the object while it is in the set.
		Node* temp = const_cast<Node*>(&(*neighbour));
		temp->gVal = cost;
		_openNodes.insert(neighbour);
		temp->parent = const_cast<Node*>(&(*_current));
	}
}

template<typename VolumeType>
float AStarPathfinder<VolumeType>::SixConnectedCost(const glm::ivec3& a, const glm::ivec3& b) {
	//This is the only heuristic I'm sure of - just use the manhatten distance for the 6-connected case.
	const uint32_t faceSteps = std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
	return float(faceSteps);
}

template<typename VolumeType>
float AStarPathfinder<VolumeType>::EighteenConnectedCost(const glm::ivec3& a, const glm::ivec3& b) {
	//I'm not sure of the correct heuristic for the 18-connected case, so I'm just letting it fall through to the
	//6-connected case. This means 'h' will be bigger than it should be, resulting in a faster path which may not
	//actually be the shortest one. If you have a correct heuristic for the 18-connected case then please let me know.

	return SixConnectedCost(a, b);
}

template<typename VolumeType>
float AStarPathfinder<VolumeType>::TwentySixConnectedCost(const glm::ivec3& a, const glm::ivec3& b) {
	//Can't say I'm certain about this heuristic - if anyone has
	//a better idea of what it should be then please let me know.
	uint32_t array[3];
	array[0] = std::abs(a.x - b.x);
	array[1] = std::abs(a.y - b.y);
	array[2] = std::abs(a.z - b.z);

	//Maybe this is better implemented directly
	//using three compares and two swaps... but not
	//until the profiler says so.
	std::sort(&array[0], &array[3]);

	const uint32_t cornerSteps = array[0];
	const uint32_t edgeSteps = array[1] - array[0];
	const uint32_t faceSteps = array[2] - array[1];

	return cornerSteps * glm::root_three<float>() + edgeSteps * glm::root_two<float>() + faceSteps;
}

template<typename VolumeType>
float AStarPathfinder<VolumeType>::computeH(const glm::ivec3& a, const glm::ivec3& b) {
	float hVal;

	switch (_params.connectivity) {
	case TwentySixConnected:
		hVal = TwentySixConnectedCost(a, b);
		break;
	case EighteenConnected:
		hVal = EighteenConnectedCost(a, b);
		break;
	case SixConnected:
		hVal = SixConnectedCost(a, b);
		break;
	default:
		hVal = 0.0f;
		core_assert_msg(false, "Connectivity parameter has an unrecognized value.");
	}

	//Sanity checks in debug mode. These can come out eventually, but I
	//want to make sure that the heuristics I've come up with make sense.
	core_assert_msg((a - b).length() > TwentySixConnectedCost(a, b), "A* heuristic error.");
	core_assert_msg(TwentySixConnectedCost(a, b) > EighteenConnectedCost(a, b), "A* heuristic error.");
	core_assert_msg(EighteenConnectedCost(a, b) > SixConnectedCost(a, b), "A* heuristic error.");

	//Apply the bias to the computed h value;
	hVal *= _params.hBias;

	//Having computed hVal, we now apply some random bias to break ties.
	//This needs to be deterministic on the input position. This random
	//bias means it is much less likely that two paths are exactly the same
	//length, and so far fewer nodes must be expanded to find the shortest path.
	//See http://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html#S12

	//Note that if the hash is zero we can have differences between the Linux vs. Windows
	//(or perhaps GCC vs. VS) versions of the code. This is probably because of the way
	//sorting inside the std::set works (i.e. one system swaps values which are identical
	//while the other one doesn't - both approaches are valid). For the same reason we want
	//to make sure that position (x,y,z) has a different hash from e.g. position (x,z,y).
	const uint32_t aX = (a.x << 16) & 0x00FF0000;
	const uint32_t aY = (a.y << 8) & 0x0000FF00;
	const uint32_t aZ = (a.z) & 0x000000FF;
	uint32_t hashVal = hash(aX | aY | aZ);

	//Stop hashVal going over 65535, and divide by 1000000 to make sure it is small.
	hashVal &= 0x0000FFFF;
	const float fHash = hashVal / 1000000.0f;

	//Apply the hash and return
	hVal += fHash;
	return hVal;
}

/**
 * Robert Jenkins' 32 bit integer hash function
 * http://www.burtleburtle.net/bob/hash/integer.html
 */
template<typename VolumeType>
uint32_t AStarPathfinder<VolumeType>::hash(uint32_t a) {
	a = (a + 0x7ed55d16) + (a << 12);
	a = (a ^ 0xc761c23c) ^ (a >> 19);
	a = (a + 0x165667b1) + (a << 5);
	a = (a + 0xd3a2646c) ^ (a << 9);
	a = (a + 0xfd7046c5) + (a << 3);
	a = (a ^ 0xb55a4f09) ^ (a >> 16);
	return a;
}

}
