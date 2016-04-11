#pragma once

#include "Clock.h"
#include "CubiquityForwardDeclarations.h"
#include "CRegion.h"
#include "CVector.h"
#include "VoxelTraits.h"
#include "Volume.h"
#include "ColoredCubicSurfaceExtractionTask.h"
#include "Octree.h"
#include "SmoothSurfaceExtractionTask.h"
#include "PolyVox/Region.h"
#include "PolyVox/Mesh.h"

#include <limits>
#include <sstream>

namespace Cubiquity {

template<typename VoxelType>
class OctreeNode {
	friend class Octree<VoxelType> ;

public:
	OctreeNode(const Region& region, uint16_t parent, Octree<VoxelType>* octree);
	~OctreeNode();

	OctreeNode* getChildNode(uint32_t childX, uint32_t childY, uint32_t childZ);
	OctreeNode* getParentNode(void);

	const ::PolyVox::Mesh<typename VoxelTraits<VoxelType>::VertexType, uint16_t>* getMesh(void);
	void setMesh(const ::PolyVox::Mesh<typename VoxelTraits<VoxelType>::VertexType, uint16_t>* mesh);

	bool isActive();
	void setActive(bool active);

	bool renderThisNode();
	void setRenderThisNode(bool render);

	bool isMeshUpToDate();
	bool isScheduledForUpdate();

	void updateFromCompletedTask(typename VoxelTraits<VoxelType>::SurfaceExtractionTaskType* completedTask);

	Region _region;
	Timestamp _dataLastModified = 2;
	Timestamp _lastSceduledForUpdate = 0;

	Timestamp _structureLastChanged = 1;
	Timestamp _propertiesLastChanged = 1;
	Timestamp _meshLastChanged = 1;
	Timestamp _nodeOrChildrenLastChanged = 1;

	Octree<VoxelType>* _octree;

	// Use flags here?

	bool _canRenderNodeOrChildren = false;
	bool _isLeaf = false;

	uint8_t _height = 0; // Zero for leaf nodes.

	typename VoxelTraits<VoxelType>::SurfaceExtractionTaskType* _lastSurfaceExtractionTask = nullptr;

	uint16_t _self = 0;
	uint16_t _children[2][2][2];

private:
	uint16_t _parent;

	bool _renderThisNode = false;
	bool _active = false;

	const ::PolyVox::Mesh<typename VoxelTraits<VoxelType>::VertexType, uint16_t>* _polyVoxMesh = nullptr;
};

// The values of these few initialisations is important
// to make sure the node is set to an 'out of date'
// state which will then try to update.
template<typename VoxelType>
OctreeNode<VoxelType>::OctreeNode(const Region& region, uint16_t parent, Octree<VoxelType>* octree) :
		_region(region), _octree(octree), _parent(parent) {
	for (int z = 0; z < 2; z++) {
		for (int y = 0; y < 2; y++) {
			for (int x = 0; x < 2; x++) {
				_children[x][y][z] = Octree<VoxelType>::InvalidNodeIndex;
			}
		}
	}
}

template<typename VoxelType>
OctreeNode<VoxelType>::~OctreeNode() {
	delete _polyVoxMesh;
}

template<typename VoxelType>
OctreeNode<VoxelType>* OctreeNode<VoxelType>::getChildNode(uint32_t childX, uint32_t childY, uint32_t childZ) {
	uint16_t childIndex = _children[childX][childY][childZ];
	if (childIndex != Octree<VoxelType>::InvalidNodeIndex) {
		OctreeNode<VoxelType>* child = _octree->_nodes[_children[childX][childY][childZ]];
		if (child->isActive()) {
			return child;
		}
	}

	return 0;
}

template<typename VoxelType>
OctreeNode<VoxelType>* OctreeNode<VoxelType>::getParentNode(void) {
	return _parent == Octree<VoxelType>::InvalidNodeIndex ? 0 : _octree->_nodes[_parent];
}

template<typename VoxelType>
const ::PolyVox::Mesh<typename VoxelTraits<VoxelType>::VertexType, uint16_t>* OctreeNode<VoxelType>::getMesh(void) {
	return _polyVoxMesh;
}

template<typename VoxelType>
void OctreeNode<VoxelType>::setMesh(const ::PolyVox::Mesh<typename VoxelTraits<VoxelType>::VertexType, uint16_t>* mesh) {
	if (_polyVoxMesh) {
		delete _polyVoxMesh;
		_polyVoxMesh = nullptr;
	}

	_polyVoxMesh = mesh;

	_meshLastChanged = Clock::getTimestamp();

	/*if (mPolyVoxMesh == 0)
	 {
	 // Force the mesh to be updated next time it is needed.
	 mDataLastModified = Clock::getTimestamp();
	 }*/
}

template<typename VoxelType>
bool OctreeNode<VoxelType>::isActive() {
	return _active;
}

template<typename VoxelType>
void OctreeNode<VoxelType>::setActive(bool active) {
	if (_active != active) {
		_active = active;

		// When a node is activated or deactivated it is the structure of the *parent*
		// which has changed (i.e. the parent has gained or lost a child (this node).
		OctreeNode<VoxelType>* parent = getParentNode();
		if (parent) {
			parent->_structureLastChanged = Clock::getTimestamp();
		}
	}
}

template<typename VoxelType>
bool OctreeNode<VoxelType>::renderThisNode() {
	return _renderThisNode;
}

template<typename VoxelType>
void OctreeNode<VoxelType>::setRenderThisNode(bool render) {
	if (_renderThisNode != render) {
		_renderThisNode = render;
		_propertiesLastChanged = Clock::getTimestamp();
	}
}

template<typename VoxelType>
bool OctreeNode<VoxelType>::isMeshUpToDate() {
	return _meshLastChanged > _dataLastModified;
}

template<typename VoxelType>
bool OctreeNode<VoxelType>::isScheduledForUpdate() {
	// We are scheduled for an update if being scheduled was the most recent thing that happened.
	return _lastSceduledForUpdate > _dataLastModified && _lastSceduledForUpdate > _meshLastChanged;
}

template<typename VoxelType>
void OctreeNode<VoxelType>::updateFromCompletedTask(typename VoxelTraits<VoxelType>::SurfaceExtractionTaskType* completedTask) {
	// Assign a new mesh if available
	/*if(completedTask->mPolyVoxMesh->getNoOfIndices() > 0)
	 {*/
	setMesh(completedTask->_polyVoxMesh);
	completedTask->_ownMesh = false; // So the task doesn't delete the mesh
	/*}
	 else // Otherwise it will just be deleted.
	 {
	 setMesh(0);
	 }*/
}

}
