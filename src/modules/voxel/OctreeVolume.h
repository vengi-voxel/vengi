/**
 * @file
 */

#pragma once

#include "polyvox/Region.h"
#include "polyvox/PagedVolume.h"
#include "Octree.h"

#define BACKGROUND_TASK_ARE_THREADED 0

#if BACKGROUND_TASK_ARE_THREADED > 0
#include "core/ConcurrentQueue.h"
#include "core/Concurrency.h"
#include <thread>
#include <list>
#endif

namespace voxel {

/**
 * @brief Octree wrapper around a PagedVolume
 */
class OctreeVolume {
public:
	class BackgroundTaskProcessor {
	private:
#if BACKGROUND_TASK_ARE_THREADED > 0
		std::atomic_bool _abort {false};
		core::ConcurrentQueue<SurfaceExtractionTask*, TaskSortCriterion> _pendingTasks;
		std::list<std::thread> _threads;

		void processTasks();
#endif
	public:
#if BACKGROUND_TASK_ARE_THREADED > 0
		BackgroundTaskProcessor(uint8_t noOfThreads = core::halfcpus());
#else
		BackgroundTaskProcessor(uint8_t noOfThreads = 1);
#endif
		~BackgroundTaskProcessor();
		void addTask(SurfaceExtractionTask* task);
	};

	/**
	 * @param[in] volume The volume that this octree manages
	 * @param[in] region The dimensions of the whole octree
	 * @param[in] baseNodeSize The minimum size of the smallest octree node in this tree
	 */
	OctreeVolume(PagedVolume* volume, const Region& region, uint32_t baseNodeSize = 32);

	const Region& getRegion() const;

	/**
	 * @note this adds a border rather than calling straight through.
	 */
	Voxel getVoxel(int32_t x, int32_t y, int32_t z) const;

	/**
	 * @note This one's a bit of a hack... direct access to underlying PolyVox volume
	 */
	PagedVolume* pagedVolume() const;

	/**
	 * @brief Octree access
	 */
	Octree& octree();

	OctreeNode* rootNode();

	/**
	 * @brief Set voxel doesn't just pass straight through, it also validates the position and marks the voxel as modified.
	 */
	void setVoxel(int32_t x, int32_t y, int32_t z, const Voxel& value, bool markAsModified);

	/**
	 * @brief Marks a region as modified so it will be regenerated later.
	 */
	void markAsModified(const Region& region);

	/**
	 * @brief Should be called before rendering a frame to update the meshes and octree structure.
	 * @param dt The milliseconds delta since last frame.
	 * @param viewPosition The position of the camera.
	 * @param lodThreshold Controls the point at which we switch to a different level of detail.
	 */
	void update(long dt, const glm::vec3& viewPosition, float lodThreshold);

	BackgroundTaskProcessor _backgroundTaskProcessor;

private:
	OctreeVolume& operator=(const OctreeVolume&);

	Region _region;
	PagedVolume* _volume;
	Octree _octree;

	// Friend functions
	friend class Octree;
	friend class OctreeNode;
};

inline const Region& OctreeVolume::getRegion() const {
	return _region;
}

inline Voxel OctreeVolume::getVoxel(int32_t x, int32_t y, int32_t z) const {
	return pagedVolume()->getVoxel(x, y, z);
}

inline PagedVolume* OctreeVolume::pagedVolume() const {
	return _volume;
}

inline Octree& OctreeVolume::octree() {
	return _octree;
}

inline OctreeNode* OctreeVolume::rootNode() {
	return octree().rootNode();
}

inline void OctreeVolume::markAsModified(const Region& region) {
	octree().markDataAsModified(region, octree().time());
}

inline void OctreeVolume::update(long dt, const glm::vec3& viewPosition, float lodThreshold) {
	octree().update(dt, viewPosition, lodThreshold);
}

}
