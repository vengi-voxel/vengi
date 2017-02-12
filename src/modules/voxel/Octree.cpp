/**
 * @file
 */

#include "Octree.h"
#include "OctreeNode.h"
#include "OctreeVolume.h"
#include "core/App.h"

#include <glm/glm.hpp>
#include <algorithm>

namespace voxel {

class PropagateTimestampsVisitor {
public:
	inline bool preChildren(OctreeNode* octreeNode) {
		// Don't actually do any work here, just make sure all children get processed.
		return true;
	}

	void postChildren(OctreeNode* octreeNode) {
		// Set timestamp to max of our own timestamps, and those of our children.
		octreeNode->_nodeOrChildrenLastChanged = std::max({
			_subtreeTimestamp,
			octreeNode->_structureLastChanged,
			octreeNode->_propertiesLastChanged,
			octreeNode->_meshLastChanged });

		// This will get propagatd back to the parent as the visitor is passed by reference.
		_subtreeTimestamp = octreeNode->_nodeOrChildrenLastChanged;
	}

private:
	// The visitor has no direct access to the children, so we
	// use this to propagate the timestamp back up to the parent.
	uint32_t _subtreeTimestamp = 0u;
};

class ScheduleUpdateIfNeededVisitor {
public:
	ScheduleUpdateIfNeededVisitor(Octree* octree, const glm::vec3& viewPosition) :
			_octree(octree), _viewPosition(viewPosition) {
	}

	bool preChildren(OctreeNode* node) {
		if (node->isMeshUpToDate()) {
			return true;
		}

		if (node->isScheduledForUpdate()) {
			return true;
		}

		const long now = _octree->time();
		SurfaceExtractionTask* task = node->_lastSurfaceExtractionTask;
		const bool extractionTask = task == nullptr || task->_processingStartedTimestamp < now;
		if (!extractionTask) {
			return true;
		}

		const bool inLodRange = node->height() <= _octree->_minimumLOD && node->height() >= _octree->_maximumLOD;
		const bool activeLod = node->isActive() && inLodRange;
		if (!activeLod) {
			return true;
		}

		node->_lastScheduledForUpdate = now;
		node->_lastSurfaceExtractionTask = new SurfaceExtractionTask(node, _octree->volume()->pagedVolume());
		// We're going to process immediatly, but the completed task will still get queued in the finished
		// queue, and we want to make sure it's the first out. So we still set a priority and make it high.
		node->_lastSurfaceExtractionTask->_priority = std::numeric_limits<uint32_t>::max();

		if (node->renderThisNode()) {
			// Still set from last frame. If we rendered it then we will probably want it again.
			_octree->_taskProcessor.addTask(node->_lastSurfaceExtractionTask);
		} else {
			_octree->volume()->_backgroundTaskProcessor.addTask(node->_lastSurfaceExtractionTask);
		}
		return true;
	}

	void postChildren(OctreeNode* octreeNode) {
	}
private:
	Octree* _octree;
	glm::vec3 _viewPosition;
};

Octree::Octree(OctreeVolume* volume, uint32_t baseNodeSize) :
		_baseNodeSize(baseNodeSize), _volume(volume) {
	_regionToCover = _volume->getRegion();
	_regionToCover.shiftUpperCorner(1, 1, 1);

	core_assert_msg(glm::isPowerOfTwo(_baseNodeSize), "Node size must be a power of two");

	const uint32_t maxDim = std::max( {
		_regionToCover.getWidthInVoxels(),
		_regionToCover.getHeightInVoxels(),
		_regionToCover.getDepthInVoxels() });

	const uint32_t octreeTargetSize = glm::ceilPowerOfTwo(maxDim);
	const uint8_t maxHeightOfTree = logBase2(octreeTargetSize / _baseNodeSize) + 1;

	const uint32_t regionToCoverWidth = _regionToCover.getWidthInVoxels();
	const uint32_t regionToCoverHeight = _regionToCover.getHeightInVoxels();
	const uint32_t regionToCoverDepth = _regionToCover.getDepthInVoxels();

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
	rootNode()->_height = maxHeightOfTree - 1;

	buildOctreeNodeTree(_rootNodeIndex);
}

Octree::~Octree() {
	_finishedExtractionTasks.abortWait();
	const size_t n = _nodes.size();
	for (size_t ct = 0; ct < n; ++ct) {
		delete _nodes[ct];
	}
}

uint16_t Octree::createNode(const Region& region, uint16_t parent) {
	OctreeNode* node = new OctreeNode(region, parent, this);

	if (parent != InvalidNodeIndex) {
		const OctreeNode* parentNode = nodeFromIndex(parent);
		core_assert_msg(parentNode->height() < 100, "Node height has gone below zero and wrapped around.");
		node->_height = parentNode->height() - 1;
	}

	_nodes.push_back(node);
	core_assert_msg(_nodes.size() < InvalidNodeIndex, "Too many octree nodes!");
	const uint16_t index = _nodes.size() - 1;
	_nodes[index]->_self = index;
	return index;
}

void Octree::update(long dt, const glm::vec3& viewPosition, float lodThreshold) {
	_time += dt;
	// This isn't a visitor because visitors only visit active nodes, and here we are setting them.
	determineActiveNodes(rootNode(), viewPosition, lodThreshold);

	acceptVisitor(ScheduleUpdateIfNeededVisitor(this, viewPosition));

	// Make sure any surface extraction tasks which were scheduled on the main thread
	// get processed before we determine what to render.
	if (_taskProcessor.hasTasks()) {
		_taskProcessor.processAllTasks();
	}

	// This will include tasks from both the background and main threads.
	while (!_finishedExtractionTasks.empty()) {
		SurfaceExtractionTask* task = nullptr;
		_finishedExtractionTasks.waitAndPop(task);
		if (task == nullptr) {
			break;
		}

		OctreeNode* node = task->_node;
		node->updateFromCompletedTask(task);

		if (node->_lastSurfaceExtractionTask == task) {
			node->_lastSurfaceExtractionTask = nullptr;
		}

		delete task;
	}

	determineWhetherToRenderNode(_rootNodeIndex);

	acceptVisitor(PropagateTimestampsVisitor());
}

void Octree::markDataAsModified(int32_t x, int32_t y, int32_t z, uint32_t newTimeStamp) {
	markAsModified(_rootNodeIndex, x, y, z, newTimeStamp);
}

void Octree::markDataAsModified(const Region& region, uint32_t newTimeStamp) {
	markAsModified(_rootNodeIndex, region, newTimeStamp);
}

void Octree::setLodRange(int32_t minimumLOD, int32_t maximumLOD) {
	core_assert_msg(minimumLOD >= maximumLOD, "Invalid LOD range. For LOD levels, the 'minimum' must be *more* than or equal to the 'maximum'");
	_minimumLOD = minimumLOD;
	_maximumLOD = maximumLOD;
}

void Octree::buildOctreeNodeTree(uint16_t parent) {
	OctreeNode* parentNode = nodeFromIndex(parent);
	const Region& parentRegion = parentNode->_region;
	core_assert_msg(parentRegion.getWidthInVoxels() == parentRegion.getHeightInVoxels(), "Region must be cubic");
	core_assert_msg(parentRegion.getWidthInVoxels() == parentRegion.getDepthInVoxels(), "Region must be cubic");

	// We know that width/height/depth are all the same.
	const uint32_t parentSize = static_cast<uint32_t>(parentRegion.getWidthInVoxels());
	if (parentSize <= _baseNodeSize) {
		return;
	}
	const glm::ivec3& baseLowerCorner = parentRegion.getLowerCorner();
	const int32_t childSize = parentRegion.getWidthInVoxels() / 2;
	const glm::ivec3 baseUpperCorner = baseLowerCorner + glm::ivec3(childSize - 1);

	for (uint8_t iz = 0u; iz < 2u; ++iz) {
		for (uint8_t iy = 0u; iy < 2u; ++iy) {
			for (uint8_t ix = 0u; ix < 2u; ++ix) {
				const glm::ivec3 offset(ix * childSize, iy * childSize, iz * childSize);
				const Region childRegion(baseLowerCorner + offset, baseUpperCorner + offset);
				if (!intersects(childRegion, _regionToCover)) {
					continue;
				}
				const uint16_t octreeNode = createNode(childRegion, parent);
				parentNode->_children[ix][iy][iz] = octreeNode;
				buildOctreeNodeTree(octreeNode);
			}
		}
	}
}

// Note - Can't this function just call the other version?
void Octree::markAsModified(uint16_t index, int32_t x, int32_t y, int32_t z, uint32_t newTimeStamp) {
	OctreeNode* node = _nodes[index];

	Region dilatedRegion = node->_region;
	dilatedRegion.grow(1); //FIXME - Think if we really need this dilation?
	if (!dilatedRegion.containsPoint(x, y, z)) {
		return;
	}

	node->_dataLastModified = newTimeStamp;

	for (uint8_t iz = 0u; iz < 2u; ++iz) {
		for (uint8_t iy = 0u; iy < 2u; ++iy) {
			for (uint8_t ix = 0u; ix < 2u; ++ix) {
				const uint16_t childIndex = node->_children[ix][iy][iz];
				if (childIndex == InvalidNodeIndex) {
					continue;
				}
				markAsModified(childIndex, x, y, z, newTimeStamp);
			}
		}
	}
}

void Octree::markAsModified(uint16_t index, const Region& region, uint32_t newTimeStamp) {
	OctreeNode* node = _nodes[index];

	if (intersects(node->_region, region)) {
		//mIsMeshUpToDate = false;
		node->_dataLastModified = newTimeStamp;

		for (uint8_t iz = 0u; iz < 2u; ++iz) {
			for (uint8_t iy = 0u; iy < 2u; ++iy) {
				for (uint8_t ix = 0u; ix < 2u; ++ix) {
					const uint16_t childIndex = node->_children[ix][iy][iz];
					if (childIndex == InvalidNodeIndex) {
						continue;
					}
					markAsModified(childIndex, region, newTimeStamp);
				}
			}
		}
	}
}

void Octree::determineActiveNodes(OctreeNode* octreeNode, const glm::vec3& viewPosition, float lodThreshold) {
	// FIXME - Should have an early out to set active to false if parent is false.

	OctreeNode* parentNode = octreeNode->getParentNode();
	if (parentNode != nullptr) {
		const glm::vec3 center(parentNode->_region.getCentre());
		const float distance = glm::length(viewPosition - center);
		const glm::vec3 diagonal(parentNode->_region.getDimensionsInCells());
		const float diagonalLength = glm::length(diagonal); // A measure of our regions size
		const float projectedSize = diagonalLength / distance;
		// As we move far away only the highest nodes will be larger than the threshold. But these may be too
		// high to ever generate meshes, so we set here a maximum height for which nodes can be set to inactive.
		const bool active = projectedSize > lodThreshold || octreeNode->_height >= _minimumLOD;
		octreeNode->setActive(active);
	} else {
		octreeNode->setActive(true);
	}

	octreeNode->_isLeaf = true;

	for (uint8_t iz = 0u; iz < 2u; ++iz) {
		for (uint8_t iy = 0u; iy < 2u; ++iy) {
			for (uint8_t ix = 0u; ix < 2u; ++ix) {
				const uint16_t childIndex = octreeNode->_children[ix][iy][iz];
				if (childIndex != InvalidNodeIndex) {
					OctreeNode* childNode = nodeFromIndex(childIndex);
					determineActiveNodes(childNode, viewPosition, lodThreshold);
				}

				// If we have (or have just created) an active and valid child then we are not a leaf.
				if (octreeNode->_isLeaf && octreeNode->getChildNode(ix, iy, iz) != nullptr) {
					octreeNode->_isLeaf = false;
				}
			}
		}
	}
}

void Octree::determineWhetherToRenderNode(uint16_t index) {
	OctreeNode* node = nodeFromIndex(index);
	if (node->_isLeaf) {
		node->_canRenderNodeOrChildren = node->isMeshUpToDate();
		node->setRenderThisNode(node->isMeshUpToDate());
		return;
	}

	bool canRenderAllChildren = true;
	for (uint8_t iz = 0u; iz < 2u; ++iz) {
		for (uint8_t iy = 0u; iy < 2u; ++iy) {
			for (uint8_t ix = 0u; ix < 2u; ++ix) {
				const uint16_t childIndex = node->_children[ix][iy][iz];
				if (childIndex == InvalidNodeIndex) {
					continue;
				}
				OctreeNode* childNode = nodeFromIndex(childIndex);
				if (childNode->isActive()) {
					determineWhetherToRenderNode(childIndex);
					canRenderAllChildren = canRenderAllChildren && childNode->_canRenderNodeOrChildren;
				} else {
					canRenderAllChildren = false;
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
		for (uint8_t iz = 0u; iz < 2u; ++iz) {
			for (uint8_t iy = 0u; iy < 2u; ++iy) {
				for (uint8_t ix = 0u; ix < 2u; ++ix) {
					OctreeNode* childNode = node->getChildNode(ix, iy, iz);
					if (childNode == nullptr) {
						continue;
					}
					childNode->setRenderThisNode(false);
				}
			}
		}

		// So we render ourself if we can
		node->setRenderThisNode(node->isMeshUpToDate());
	}
}

}
