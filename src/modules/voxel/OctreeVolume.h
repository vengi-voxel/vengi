#pragma once

#include "polyvox/Region.h"
#include "polyvox/PagedVolume.h"
#include "core/App.h"
#include "core/ConcurrentQueue.h"
#include "core/Concurrency.h"
#include "Octree.h"
#include <thread>
#include <list>

namespace voxel {

class OctreeVolume {
public:
	class BackgroundTaskProcessor {
	private:
		std::atomic_bool _abort {false};
	public:
		BackgroundTaskProcessor(uint32_t noOfThreads = core::halfcpus()) {
			for (uint32_t ct = 0; ct < noOfThreads; ++ct) {
				_threads.emplace_back(std::bind(&BackgroundTaskProcessor::processTasks, this));
			}
		}

		~BackgroundTaskProcessor() {
			_abort = true;
			_pendingTasks.abortWait();
			for (auto i = _threads.begin(); i != _threads.end(); ++i) {
				i->join();
			}
		}

		void addTask(SurfaceExtractionTask* task) {
			_pendingTasks.push(task);
		}

		void processTasks(void) {
			while (!_abort) {
				SurfaceExtractionTask* task = nullptr;
				_pendingTasks.waitAndPop(task);
				if (task != nullptr) {
					task->process();
				}
			}
		}

		core::ConcurrentQueue<SurfaceExtractionTask*, TaskSortCriterion> _pendingTasks;
		std::list<std::thread> _threads;
	};

	OctreeVolume(PagedVolume* volume, const Region& region, uint32_t baseNodeSize) :
			_region(region), _volume(volume), _octree(this, baseNodeSize) {
	}

	inline const Region& getRegion() const {
		return _region;
	}

	// Note this adds a border rather than calling straight through.
	inline Voxel getVoxel(int32_t x, int32_t y, int32_t z) const {
		return _volume->getVoxel(x, y, z);
	}

	// This one's a bit of a hack... direct access to underlying PolyVox volume
	inline PagedVolume* _getPolyVoxOctreeVolume() const {
		return _volume;
	}

	// Octree access
	inline Octree& getOctree() {
		return _octree;
	}

	inline OctreeNode* getRootOctreeNode() {
		return _octree.getRootNode();
	}

	// Set voxel doesn't just pass straight through, it also validates the position and marks the voxel as modified.
	void setVoxel(int32_t x, int32_t y, int32_t z, const Voxel& value, bool markAsModified) {
		core_assert_msg(_region.containsPoint(x, y, z), "Attempted to write to a voxel which is outside of the volume");
		_volume->setVoxel(x, y, z, value);
		if (markAsModified) {
			_octree.markDataAsModified(x, y, z, core::App::getInstance()->timeProvider()->currentTime());
		}
	}

	// Marks a region as modified so it will be regenerated later.
	inline void markAsModified(const Region& region) {
		_octree.markDataAsModified(region, core::App::getInstance()->timeProvider()->currentTime());
	}

	// Should be called before rendering a frame to update the meshes and octree structure.
	inline void update(const glm::vec3& viewPosition, float lodThreshold) {
		_octree.update(viewPosition, lodThreshold);
	}

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

}
