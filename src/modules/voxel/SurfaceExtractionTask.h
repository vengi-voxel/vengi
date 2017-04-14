#pragma once

#include "voxel/polyvox/PagedVolume.h"
#include <memory>
#include <limits>

namespace voxel {

class OctreeNode;
class Mesh;

class SurfaceExtractionTask {
public:
	SurfaceExtractionTask(OctreeNode* node, PagedVolume* volume);
	~SurfaceExtractionTask();

	// Extract the surface
	void process();

public:
	int _priority = 0;
	OctreeNode* _node;
	PagedVolume* _volume;
	std::shared_ptr<Mesh> _mesh;
	std::shared_ptr<Mesh> _meshWater;
	long _processingStartedTimestamp = std::numeric_limits<long>::max();
};

struct TaskSortCriterion {
	inline bool operator()(const SurfaceExtractionTask* task1, const SurfaceExtractionTask* task2) const {
		return task1->_priority < task2->_priority;
	}
};

}
