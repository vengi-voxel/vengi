#pragma once

#include "Clock.h"
#include "ConcurrentQueue.h"
#include "CubiquityForwardDeclarations.h"
#include "CRegion.h"
#include "Task.h"
#include "CVector.h"
#include "VoxelTraits.h"
#include "core/Common.h"
#include "OctreeNode.h"
#include "Volume.h"
#include "MainThreadTaskProcessor.h"

#include <algorithm>
#include <vector>

namespace Cubiquity {

namespace OctreeConstructionModes {

enum OctreeConstructionMode {
	BoundVoxels = 0, BoundCells = 1
};

}

typedef OctreeConstructionModes::OctreeConstructionMode OctreeConstructionMode;

template<typename VoxelType>
class Octree {
	friend class OctreeNode<VoxelType> ;

public:
	static const uint16_t InvalidNodeIndex = 0xFFFF;

	Octree(Volume<VoxelType>* volume, OctreeConstructionMode octreeConstructionMode, unsigned int baseNodeSize);
	~Octree();

	template<typename VisitorType>
	void acceptVisitor(VisitorType visitor) {
		visitNode(getRootNode(), visitor);
	}

	OctreeNode<VoxelType>* getRootNode(void) {
		return _nodes[_rootNodeIndex];
	}

	Volume<VoxelType>* getVolume(void) {
		return _volume;
	}

	// This one feels hacky?
	OctreeNode<VoxelType>* getNodeFromIndex(uint16_t index) {
		return _nodes[index];
	}

	bool update(const Vector3F& viewPosition, float lodThreshold);

	void markDataAsModified(int32_t x, int32_t y, int32_t z, Timestamp newTimeStamp);
	void markDataAsModified(const Region& region, Timestamp newTimeStamp);

	void buildOctreeNodeTree(uint16_t parent);
	void determineActiveNodes(OctreeNode<VoxelType>* octreeNode, const Vector3F& viewPosition, float lodThreshold);

	concurrent_queue<typename VoxelTraits<VoxelType>::SurfaceExtractionTaskType*, TaskSortCriterion> _finishedSurfaceExtractionTasks;

	void setLodRange(int32_t minimumLOD, int32_t maximumLOD);

	// Note that the maximum LOD refers to the *most detailed* LOD, which is actually the *smallest* height
	// in the octree (the greatest depth). If confused, think how texture mipmapping works, where the most
	// detailed MIP is number zero. Level zero is the raw voxel data and successive levels downsample it.
	int32_t _maximumLOD = 0;
	int32_t _minimumLOD = 2;// Must be *more* than maximum

private:
	uint16_t createNode(Region region, uint16_t parent);

	template<typename VisitorType>
	void visitNode(OctreeNode<VoxelType>* node, VisitorType& visitor);

	void markAsModified(uint16_t index, int32_t x, int32_t y, int32_t z, Timestamp newTimeStamp);
	void markAsModified(uint16_t index, const Region& region, Timestamp newTimeStamp);

	Timestamp propagateTimestamps(uint16_t index);

	void scheduleUpdateIfNeeded(OctreeNode<VoxelType>* node, const Vector3F& viewPosition);

	void determineWhetherToRenderNode(uint16_t index);

	std::vector<OctreeNode<VoxelType>*> _nodes;

	uint16_t _rootNodeIndex = InvalidNodeIndex;
	const unsigned int _baseNodeSize;

	Volume<VoxelType>* _volume;

	// The extent of the octree may be significantly larger than the volume, but we only want to
	// create nodes which actually overlap the volume (otherwise they are guaranteed to be empty).
	Region _regionToCover;

	OctreeConstructionMode _octreeConstructionMode;
};

template<typename VoxelType>
class PropagateTimestampsVisitor {
public:
	PropagateTimestampsVisitor() {
		_subtreeTimestamp = 0;
	}

	bool preChildren(OctreeNode<VoxelType>* octreeNode) {
		// Don't actually do any work here, just make sure all children get processed.
		return true;
	}

	void postChildren(OctreeNode<VoxelType>* octreeNode) {
		// Set timestamp to max of our own timestamps, and those of our children.
		octreeNode->_nodeOrChildrenLastChanged = std::max({
			_subtreeTimestamp,
			octreeNode->_structureLastChanged,
			octreeNode->_propertiesLastChanged,
			octreeNode->_meshLastChanged
		});

		// This will get propagatd back to the parent as the visitor is passed by reference.
		_subtreeTimestamp = octreeNode->_nodeOrChildrenLastChanged;
	}

private:
	// The visitor has no direct access to the children, so we
	// use this to propagate the timestamp back up to the parent.
	Timestamp _subtreeTimestamp;
};

template<typename VoxelType>
class ScheduleUpdateIfNeededVisitor {
public:
	ScheduleUpdateIfNeededVisitor(const Vector3F& viewPosition) :
			_viewPosition(viewPosition) {
	}

	bool preChildren(OctreeNode<VoxelType>* octreeNode) {
		// Remember that min and max are counter-intuitive here!
		if (!octreeNode->isMeshUpToDate()
				&& !octreeNode->isScheduledForUpdate()
				&& (octreeNode->_lastSurfaceExtractionTask == 0 || octreeNode->_lastSurfaceExtractionTask->_processingStartedTimestamp < Clock::getTimestamp())
				&& (octreeNode->isActive() && octreeNode->_height <= octreeNode->_octree->_minimumLOD
					&& (octreeNode->_height >= octreeNode->_octree->_maximumLOD))) {
			octreeNode->_lastSceduledForUpdate = Clock::getTimestamp();

			octreeNode->_lastSurfaceExtractionTask = new typename VoxelTraits<VoxelType>::SurfaceExtractionTaskType(octreeNode,
					octreeNode->_octree->getVolume()->_getPolyVoxVolume());

			// If the node was rendered last frame then this update is probably the result of an editing operation, rather than
			// the node only just becoming visible. For editing operations it is important to process them immediatly so that we
			// don't see temporary cracks in the mesh as different parts up updated at different times.
			//if(node->mExtractOnMainThread) //This flag should still be set from last frame.
			// We're going to process immediatly, but the completed task will still get queued in the finished
			// queue, and we want to make sure it's the first out. So we still set a priority and make it high.
			octreeNode->_lastSurfaceExtractionTask->priority = (std::numeric_limits<uint32_t>::max)();

			// Still set from last frame. If we rendered it then we will probably want it again.
			if (octreeNode->renderThisNode()) {
				gMainThreadTaskProcessor.addTask(octreeNode->_lastSurfaceExtractionTask);
			} else {
				octreeNode->_octree->getVolume()->_backgroundTaskProcessor->addTask(octreeNode->_lastSurfaceExtractionTask);
			}
		}

		return true;
	}

	void postChildren(OctreeNode<VoxelType>* octreeNode) {
	}

private:
	Vector3F _viewPosition;
};

template<typename VoxelType>
Octree<VoxelType>::Octree(Volume<VoxelType>* volume, OctreeConstructionMode octreeConstructionMode, unsigned int baseNodeSize) :
		_baseNodeSize(baseNodeSize), _volume(volume), _octreeConstructionMode(octreeConstructionMode)
{
	_regionToCover = _volume->getEnclosingRegion();
	if (_octreeConstructionMode == OctreeConstructionModes::BoundVoxels) {
		_regionToCover.shiftUpperCorner(1, 1, 1);
	} else if (_octreeConstructionMode == OctreeConstructionModes::BoundCells) {
		_regionToCover.shiftLowerCorner(-1, -1, -1);
		_regionToCover.shiftUpperCorner(1, 1, 1);
	}

	core_assert_msg(::PolyVox::isPowerOf2(_baseNodeSize), "Node size must be a power of two");

	uint32_t largestVolumeDimension = std::max(_regionToCover.getWidthInVoxels(), std::max(_regionToCover.getHeightInVoxels(), _regionToCover.getDepthInVoxels()));
	if (_octreeConstructionMode == OctreeConstructionModes::BoundCells) {
		largestVolumeDimension--;
	}

	uint32_t octreeTargetSize = ::PolyVox::upperPowerOfTwo(largestVolumeDimension);

	uint8_t maxHeightOfTree = ::PolyVox::logBase2((octreeTargetSize) / _baseNodeSize) + 1;

	uint32_t regionToCoverWidth = (_octreeConstructionMode == OctreeConstructionModes::BoundCells) ? _regionToCover.getWidthInCells() : _regionToCover.getWidthInVoxels();
	uint32_t regionToCoverHeight = (_octreeConstructionMode == OctreeConstructionModes::BoundCells) ? _regionToCover.getHeightInCells() : _regionToCover.getHeightInVoxels();
	uint32_t regionToCoverDepth = (_octreeConstructionMode == OctreeConstructionModes::BoundCells) ? _regionToCover.getDepthInCells() : _regionToCover.getDepthInVoxels();

	uint32_t widthIncrease = octreeTargetSize - regionToCoverWidth;
	uint32_t heightIncrease = octreeTargetSize - regionToCoverHeight;
	uint32_t depthIncrease = octreeTargetSize - regionToCoverDepth;

	Region octreeRegion(_regionToCover);

	if (widthIncrease % 2 == 1) {
		octreeRegion.setUpperX(octreeRegion.getUpperX() + 1);
		--widthIncrease;
	}

	if (heightIncrease % 2 == 1) {
		octreeRegion.setUpperY(octreeRegion.getUpperY() + 1);
		--heightIncrease;
	}
	if (depthIncrease % 2 == 1) {
		octreeRegion.setUpperZ(octreeRegion.getUpperZ() + 1);
		--depthIncrease;
	}

	octreeRegion.grow(widthIncrease / 2, heightIncrease / 2, depthIncrease / 2);

	_rootNodeIndex = createNode(octreeRegion, InvalidNodeIndex);
	_nodes[_rootNodeIndex]->_height = maxHeightOfTree - 1;

	buildOctreeNodeTree(_rootNodeIndex);
}

template<typename VoxelType>
Octree<VoxelType>::~Octree() {
	for (uint32_t ct = 0; ct < _nodes.size(); ct++) {
		delete _nodes[ct];
	}
}

template<typename VoxelType>
uint16_t Octree<VoxelType>::createNode(Region region, uint16_t parent) {
	OctreeNode<VoxelType>* node = new OctreeNode<VoxelType>(region, parent, this);

	if (parent != InvalidNodeIndex) {
		core_assert_msg(_nodes[parent]->_height < 100, "Node height has gone below zero and wrapped around.");
		node->_height = _nodes[parent]->_height - 1;
	}

	_nodes.push_back(node);
	core_assert_msg(_nodes.size() < InvalidNodeIndex, "Too many octree nodes!");
	uint16_t index = _nodes.size() - 1;
	_nodes[index]->_self = index;
	return index;
}

template<typename VoxelType>
bool Octree<VoxelType>::update(const Vector3F& viewPosition, float lodThreshold) {
	// This isn't a vistior because visitors only visit active nodes, and here we are setting them.
	determineActiveNodes(getRootNode(), viewPosition, lodThreshold);

	acceptVisitor(ScheduleUpdateIfNeededVisitor<VoxelType>(viewPosition));

	// Make sure any surface extraction tasks which were scheduled on the main thread get processed before we determine what to render.
	if (gMainThreadTaskProcessor.hasTasks()) {
		gMainThreadTaskProcessor.processAllTasks(); // Doesn't really belong here
	} else {
		getVolume()->_backgroundTaskProcessor->processOneTask(); // Doesn't really belong here
	}

	// This will include tasks from both the background and main threads.
	while (!_finishedSurfaceExtractionTasks.empty()) {
		typename VoxelTraits<VoxelType>::SurfaceExtractionTaskType* task;
		_finishedSurfaceExtractionTasks.wait_and_pop(task);

		task->_octreeNode->updateFromCompletedTask(task);

		if (task->_octreeNode->_lastSurfaceExtractionTask == task) {
			task->_octreeNode->_lastSurfaceExtractionTask = 0;
		}

		delete task;
	}

	//acceptVisitor(DetermineWhetherToRenderVisitor<VoxelType>());

	determineWhetherToRenderNode(_rootNodeIndex);

	acceptVisitor(PropagateTimestampsVisitor<VoxelType>());

	// If there are no pending tasks then return truem to indicate we are up to date.
	return !gMainThreadTaskProcessor.hasTasks() && !getVolume()->_backgroundTaskProcessor->hasTasks();
}

template<typename VoxelType>
void Octree<VoxelType>::markDataAsModified(int32_t x, int32_t y, int32_t z, Timestamp newTimeStamp) {
	markAsModified(_rootNodeIndex, x, y, z, newTimeStamp);
}

template<typename VoxelType>
void Octree<VoxelType>::markDataAsModified(const Region& region, Timestamp newTimeStamp) {
	markAsModified(_rootNodeIndex, region, newTimeStamp);
}

template<typename VoxelType>
void Octree<VoxelType>::setLodRange(int32_t minimumLOD, int32_t maximumLOD) {
	core_assert_msg(minimumLOD < maximumLOD, "Invalid LOD range. For LOD levels, the 'minimum' must be *more* than or equal to the 'maximum'");
	_minimumLOD = minimumLOD;
	_maximumLOD = maximumLOD;
}

template<typename VoxelType>
void Octree<VoxelType>::buildOctreeNodeTree(uint16_t parent) {
	core_assert_msg(_nodes[parent]->_region.getWidthInVoxels() == _nodes[parent]->_region.getHeightInVoxels(), "Region must be cubic");
	core_assert_msg(_nodes[parent]->_region.getWidthInVoxels() == _nodes[parent]->_region.getDepthInVoxels(), "Region must be cubic");

	//We know that width/height/depth are all the same.
	const uint32_t parentSize = static_cast<uint32_t>(
			(_octreeConstructionMode == OctreeConstructionModes::BoundCells) ? _nodes[parent]->_region.getWidthInCells() : _nodes[parent]->_region.getWidthInVoxels());

	if (parentSize <= _baseNodeSize) {
		return;
	}
	const Vector3I baseLowerCorner = _nodes[parent]->_region.getLowerCorner();
	const int32_t childSize =
			(_octreeConstructionMode == OctreeConstructionModes::BoundCells) ? _nodes[parent]->_region.getWidthInCells() / 2 : _nodes[parent]->_region.getWidthInVoxels() / 2;

	Vector3I baseUpperCorner;
	if (_octreeConstructionMode == OctreeConstructionModes::BoundCells) {
		baseUpperCorner = baseLowerCorner + Vector3I(childSize, childSize, childSize);
	} else {
		baseUpperCorner = baseLowerCorner + Vector3I(childSize - 1, childSize - 1, childSize - 1);
	}

	for (int z = 0; z < 2; z++) {
		for (int y = 0; y < 2; y++) {
			for (int x = 0; x < 2; x++) {
				Vector3I offset(x * childSize, y * childSize, z * childSize);
				Region childRegion(baseLowerCorner + offset, baseUpperCorner + offset);
				if (intersects(childRegion, _regionToCover)) {
					uint16_t octreeNode = createNode(childRegion, parent);
					_nodes[parent]->_children[x][y][z] = octreeNode;
					buildOctreeNodeTree(octreeNode);
				}
			}
		}
	}
}

template<typename VoxelType>
void Octree<VoxelType>::markAsModified(uint16_t index, int32_t x, int32_t y, int32_t z, Timestamp newTimeStamp) {
	// Note - Can't this function just call the other version?
	OctreeNode<VoxelType>* node = _nodes[index];

	Region dilatedRegion = node->_region;
	dilatedRegion.grow(1); //FIXME - Think if we really need this dilation?

	if (!dilatedRegion.containsPoint(x, y, z)) {
		return;
	}
	//mIsMeshUpToDate = false;
	node->_dataLastModified = newTimeStamp;

	for (int iz = 0; iz < 2; iz++) {
		for (int iy = 0; iy < 2; iy++) {
			for (int ix = 0; ix < 2; ix++) {
				uint16_t childIndex = node->_children[ix][iy][iz];
				if (childIndex != InvalidNodeIndex) {
					markAsModified(childIndex, x, y, z, newTimeStamp);
				}
			}
		}
	}
}

template<typename VoxelType>
void Octree<VoxelType>::markAsModified(uint16_t index, const Region& region, Timestamp newTimeStamp) {
	OctreeNode<VoxelType>* node = _nodes[index];

	if (!intersects(node->_region, region)) {
		return;
	}

	//mIsMeshUpToDate = false;
	node->_dataLastModified = newTimeStamp;

	for (int iz = 0; iz < 2; iz++) {
		for (int iy = 0; iy < 2; iy++) {
			for (int ix = 0; ix < 2; ix++) {
				uint16_t childIndex = node->_children[ix][iy][iz];
				if (childIndex != InvalidNodeIndex) {
					markAsModified(childIndex, region, newTimeStamp);
				}
			}
		}
	}
}

template<typename VoxelType>
void Octree<VoxelType>::determineActiveNodes(OctreeNode<VoxelType>* octreeNode, const Vector3F& viewPosition, float lodThreshold) {
	// FIXME - Should have an early out to set active to false if parent is false.

	OctreeNode<VoxelType>* parentNode = octreeNode->getParentNode();
	if (parentNode) {
		const Vector3F regionCentre = static_cast<Vector3F>(parentNode->_region.getCentre());
		const float distance = (viewPosition - regionCentre).length();
		const Vector3I diagonal = parentNode->_region.getUpperCorner() - parentNode->_region.getLowerCorner();
		const float diagonalLength = diagonal.length(); // A measure of our regions size
		const float projectedSize = diagonalLength / distance;
		// As we move far away only the highest nodes will be larger than the threshold. But these may be too
		// high to ever generate meshes, so we set here a maximum height for which nodes can be set to inacive.
		const bool active = (projectedSize > lodThreshold) || (octreeNode->_height >= _minimumLOD);
		octreeNode->setActive(active);
	} else {
		octreeNode->setActive(true);
	}

	octreeNode->_isLeaf = true;

	for (int iz = 0; iz < 2; iz++) {
		for (int iy = 0; iy < 2; iy++) {
			for (int ix = 0; ix < 2; ix++) {
				uint16_t childIndex = octreeNode->_children[ix][iy][iz];
				if (childIndex != InvalidNodeIndex) {
					OctreeNode<VoxelType>* childNode = _nodes[childIndex];
					determineActiveNodes(childNode, viewPosition, lodThreshold);
				}

				// If we have (or have just created) an active and valid child then we are not a leaf.
				if (octreeNode->getChildNode(ix, iy, iz)) {
					octreeNode->_isLeaf = false;
				}
			}
		}
	}
}

template<typename VoxelType>
void Octree<VoxelType>::determineWhetherToRenderNode(uint16_t index) {
	OctreeNode<VoxelType>* node = _nodes[index];
	if (node->_isLeaf) {
		node->_canRenderNodeOrChildren = node->isMeshUpToDate();
		node->setRenderThisNode(node->isMeshUpToDate());
		return;
	}

	bool canRenderAllChildren = true;
	for (int iz = 0; iz < 2; iz++) {
		for (int iy = 0; iy < 2; iy++) {
			for (int ix = 0; ix < 2; ix++) {
				uint16_t childIndex = node->_children[ix][iy][iz];
				if (childIndex != InvalidNodeIndex) {
					OctreeNode<VoxelType>* childNode = _nodes[childIndex];
					if (childNode->isActive()) {
						determineWhetherToRenderNode(childIndex);
						canRenderAllChildren = canRenderAllChildren && childNode->_canRenderNodeOrChildren;
					} else {
						canRenderAllChildren = false;
					}
				}
			}
		}
	}

	node->_canRenderNodeOrChildren = node->isMeshUpToDate() | canRenderAllChildren;

	if (canRenderAllChildren) {
		// If we can render all the children then don't render ourself.
		node->setRenderThisNode(false);
	} else {
		// As we can't render all children then we must render no children.
		for (int iz = 0; iz < 2; iz++) {
			for (int iy = 0; iy < 2; iy++) {
				for (int ix = 0; ix < 2; ix++) {
					OctreeNode<VoxelType>* childNode = node->getChildNode(ix, iy, iz);
					if (childNode) {
						childNode->setRenderThisNode(false);
					}
				}
			}
		}

		// So we render ourself if we can
		node->setRenderThisNode(node->isMeshUpToDate());
	}
}

template<typename VoxelType>
template<typename VisitorType>
void Octree<VoxelType>::visitNode(OctreeNode<VoxelType>* node, VisitorType& visitor) {
	const bool processChildren = visitor.preChildren(node);

	if (processChildren) {
		for (int iz = 0; iz < 2; iz++) {
			for (int iy = 0; iy < 2; iy++) {
				for (int ix = 0; ix < 2; ix++) {
					OctreeNode<VoxelType>* childNode = node->getChildNode(ix, iy, iz);
					if (childNode) {
						visitNode(childNode, visitor);
					}
				}
			}
		}
	}

	visitor.postChildren(node);
}

}
