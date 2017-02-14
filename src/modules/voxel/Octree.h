/**
 * @file
 */

#pragma once

#include "polyvox/Region.h"
#include "polyvox/PagedVolume.h"
#include "SurfaceExtractionTask.h"
#include "core/ConcurrentQueue.h"
#include "OctreeNode.h"

#include <vector>

namespace voxel {

class OctreeVolume;

class Octree {
	friend class OctreeNode;
	friend class SurfaceExtractionTask;

public:
	static const NodeIndex InvalidNodeIndex = 0xFFFF;

	/**
	 * @param[in] baseNodeSize The minimum size of the smallest octree node in this tree
	 */
	Octree(OctreeVolume* volume, uint32_t baseNodeSize);
	~Octree();

	OctreeNode* rootNode() const;

	OctreeVolume* volume() const;

	/**
	 * @param lodThreshold Controls the point at which we switch to a different level of detail.
	 */
	void update(long dt, const glm::vec3& viewPosition, float lodThreshold);

	long time() const;

	void markDataAsModified(int32_t x, int32_t y, int32_t z, uint32_t newTimeStamp);
	void markDataAsModified(const Region& region, uint32_t newTimeStamp);

	/**
	 * @note For LOD levels, the 'minimum' must be *more* than or equal to the 'maximum'
	 * @param minimumLOD Specifies the lowest (least detailed) LOD which we render for this volume.
	 *
	 * @note
	 * Note that the maximum LOD refers to the *most detailed* LOD, which is actually the *smallest* height
	 * in the octree (the greatest depth). If confused, think how texture mipmapping works, where the most
	 * detailed MIP is number zero. Level zero is the raw voxel data and successive levels downsample it.
	 */
	void setLodRange(int32_t minimumLOD, int32_t maximumLOD);
	int32_t maximumLOD() const;
	int32_t minimumLOD() const;

	class MainThreadTaskProcessor {
	private:
		std::list<SurfaceExtractionTask*> _pendingTasks;
	public:
		~MainThreadTaskProcessor();

		void addTask(SurfaceExtractionTask* task);
		bool hasTasks() const;
		bool processOneTask();
		void processAllTasks();
	};
	MainThreadTaskProcessor _taskProcessor;

private:
	void buildOctreeNodeTree(NodeIndex parent);
	void determineActiveNodes(OctreeNode* octreeNode, const glm::vec3& viewPosition, float lodThreshold);

	OctreeNode* nodeFromIndex(NodeIndex index) const;

	NodeIndex createNode(const Region& region, NodeIndex parent);

	/**
	 * @brief Traverses the tree
	 * @note The given VisitorType must implement a preChildren() and a postChildren() method
	 */
	template<typename VisitorType>
	void acceptVisitor(VisitorType&& visitor) {
		visitNode(rootNode(), std::forward<VisitorType>(visitor));
	}

	template<typename VisitorType>
	void visitNode(OctreeNode* node, VisitorType&& visitor)  {
		const bool processChildren = visitor.preChildren(node);

		if (processChildren) {
			for (uint8_t iz = 0u; iz < 2u; ++iz) {
				for (uint8_t iy = 0u; iy < 2u; ++iy) {
					for (uint8_t ix = 0u; ix < 2u; ++ix) {
						OctreeNode* childNode = node->getActiveChildNode(ix, iy, iz);
						if (childNode) {
							visitNode(childNode, visitor);
						}
					}
				}
			}
		}
		visitor.postChildren(node);
	}

	void markAsModified(NodeIndex index, int32_t x, int32_t y, int32_t z, uint32_t newTimeStamp);
	void markAsModified(NodeIndex index, const Region& region, uint32_t newTimeStamp);

	void determineWhetherToRenderNode(NodeIndex index);

	std::vector<OctreeNode*> _nodes;

	NodeIndex _rootNodeIndex = InvalidNodeIndex;
	const uint32_t _baseNodeSize;
	long _time = 1000l;

	int32_t _maximumLOD = 0;
	/**
	 * @note Must be *more* than maximum
	 */
	int32_t _minimumLOD = 2;

	OctreeVolume* _volume;

	core::ConcurrentQueue<SurfaceExtractionTask*> _finishedExtractionTasks;

	/**
	 * @brief The extent of the octree may be significantly larger than the volume, but we only want to
	 * create nodes which actually overlap the volume (otherwise they are guaranteed to be empty).
	 */
	Region _regionToCover;
};

inline int32_t Octree::maximumLOD() const {
	return _maximumLOD;
}

inline int32_t Octree::minimumLOD() const {
	return _minimumLOD;
}

inline long Octree::time() const {
	return _time;
}

inline OctreeNode* Octree::nodeFromIndex(NodeIndex index) const {
	core_assert(index != InvalidNodeIndex);
	return _nodes[index];
}

inline OctreeNode* Octree::rootNode() const {
	return nodeFromIndex(_rootNodeIndex);
}

inline OctreeVolume* Octree::volume() const {
	return _volume;
}

}
