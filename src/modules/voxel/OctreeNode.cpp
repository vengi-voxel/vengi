/**
 * @file
 */

#include "OctreeNode.h"
#include "Octree.h"
#include "polyvox/PagedVolume.h"

#include <limits>
#include <sstream>

namespace voxel {

OctreeNode::OctreeNode(const Region& region, uint16_t parent, Octree* octree) :
		_region(region), _octree(octree), _parent(parent) {
	for (uint8_t z = 0; z < 2; ++z) {
		for (uint8_t y = 0; y < 2; ++y) {
			for (uint8_t x = 0; x < 2; ++x) {
				_children[x][y][z] = Octree::InvalidNodeIndex;
			}
		}
	}
}

OctreeNode* OctreeNode::getChildNode(uint8_t x, uint8_t y, uint8_t z) const {
	const uint16_t index = _children[x][y][z];
	if (index == voxel::Octree::InvalidNodeIndex) {
		return nullptr;
	}
	voxel::OctreeNode* child = _octree->nodeFromIndex(index);
	return child;
}

OctreeNode* OctreeNode::getActiveChildNode(uint32_t childX, uint32_t childY, uint32_t childZ) const {
	OctreeNode* child = getChildNode(childX, childY, childZ);
	if (!child->isActive()) {
		return nullptr;
	}
	return child;
}

OctreeNode* OctreeNode::getParentNode() const {
	return _parent == Octree::InvalidNodeIndex ? nullptr : _octree->_nodes[_parent];
}

const Mesh* OctreeNode::getMesh() {
	return _mesh.get();
}

const Mesh* OctreeNode::getWaterMesh() {
	return _waterMesh.get();
}

void OctreeNode::setMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Mesh>& waterMesh) {
	_mesh = mesh;
	_waterMesh = waterMesh;
	_meshLastChanged = _octree->_time;
}

void OctreeNode::setActive(bool active) {
	if (_active == active) {
		return;
	}
	_active = active;

	// When a node is activated or deactivated it is the structure of the *parent*
	// which has changed (i.e. the parent has gained or lost a child (this node).
	OctreeNode* parent = getParentNode();
	if (parent != nullptr) {
		parent->_structureLastChanged = _octree->_time;
	}
}

void OctreeNode::setRenderThisNode(bool render) {
	if (_renderThisNode == render) {
		return;
	}
	_renderThisNode = render;
	_propertiesLastChanged = _octree->_time;
}

bool OctreeNode::isMeshUpToDate() const {
	return _meshLastChanged > _dataLastModified;
}

bool OctreeNode::isScheduledForUpdate() const {
	return _lastScheduledForUpdate > _dataLastModified && _lastScheduledForUpdate > _meshLastChanged;
}

void OctreeNode::updateFromCompletedTask(SurfaceExtractionTask* completedTask) {
	setMesh(completedTask->_mesh, completedTask->_meshWater);
}

}
