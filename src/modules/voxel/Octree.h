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

public:
	static const uint16_t InvalidNodeIndex = 0xFFFF;

	Octree(OctreeVolume* volume, unsigned int baseNodeSize);
	~Octree();

	template<typename VisitorType>
	void acceptVisitor(VisitorType visitor) {
		visitNode(getRootNode(), visitor);
	}

	OctreeNode* getRootNode() const {
		return _nodes[_rootNodeIndex];
	}

	OctreeVolume* getVolume() const {
		return _volume;
	}

	// This one feels hacky?
	OctreeNode* getNodeFromIndex(uint16_t index) const {
		return _nodes[index];
	}

	void update(const glm::vec3& viewPosition, float lodThreshold);

	void markDataAsModified(int32_t x, int32_t y, int32_t z, uint32_t newTimeStamp);
	void markDataAsModified(const Region& region, uint32_t newTimeStamp);

	void buildOctreeNodeTree(uint16_t parent);
	void determineActiveNodes(OctreeNode* octreeNode, const glm::vec3& viewPosition, float lodThreshold);

	core::ConcurrentQueue<SurfaceExtractionTask*> _finishedExtractionTasks;

	void setLodRange(int32_t minimumLOD, int32_t maximumLOD);

	// Note that the maximum LOD refers to the *most detailed* LOD, which is actually the *smallest* height
	// in the octree (the greatest depth). If confused, think how texture mipmapping works, where the most
	// detailed MIP is number zero. Level zero is the raw voxel data and successive levels downsample it.
	int32_t _maximumLOD = 0;
	 // Must be *more* than maximum
	int32_t _minimumLOD = 2;
	class MainThreadTaskProcessor {
	private:
		std::list<SurfaceExtractionTask*> _pendingTasks;
	public:
		~MainThreadTaskProcessor() {
			_pendingTasks.clear();
		}

		void addTask(SurfaceExtractionTask* task) {
			_pendingTasks.push_back(task);
		}

		inline bool hasTasks() const {
			return !_pendingTasks.empty();
		}

		void processOneTask() {
			if (!hasTasks()) {
				return;
			}
			SurfaceExtractionTask* task = _pendingTasks.front();
			_pendingTasks.pop_front();
			task->process();
		}

		void processAllTasks() {
			while (hasTasks()) {
				SurfaceExtractionTask* task = _pendingTasks.front();
				_pendingTasks.pop_front();
				task->process();
			}
		}
	};
	MainThreadTaskProcessor _taskProcessor;

private:
	uint16_t createNode(const Region& region, uint16_t parent);

	template<typename VisitorType>
	void visitNode(OctreeNode* node, VisitorType& visitor)  {
		const bool processChildren = visitor.preChildren(node);

		if (processChildren) {
			for (int iz = 0; iz < 2; iz++) {
				for (int iy = 0; iy < 2; iy++) {
					for (int ix = 0; ix < 2; ix++) {
						OctreeNode* childNode = node->getChildNode(ix, iy, iz);
						if (childNode) {
							visitNode(childNode, visitor);
						}
					}
				}
			}
		}
		visitor.postChildren(node);
	}

	void markAsModified(uint16_t index, int32_t x, int32_t y, int32_t z, uint32_t newTimeStamp);
	void markAsModified(uint16_t index, const Region& region, uint32_t newTimeStamp);

	void determineWhetherToRenderNode(uint16_t index);

	std::vector<OctreeNode*> _nodes;

	uint16_t _rootNodeIndex = InvalidNodeIndex;
	const unsigned int _baseNodeSize;

	OctreeVolume* _volume;

	// The extent of the octree may be significantly larger than the volume, but we only want to
	// create nodes which actually overlap the volume (otherwise they are guaranteed to be empty).
	Region _regionToCover;
};

}
