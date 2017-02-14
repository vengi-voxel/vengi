/**
 * @file
 */

#include "OctreeVolume.h"

namespace voxel {

#if BACKGROUND_TASK_ARE_THREADED > 0
void OctreeVolume::BackgroundTaskProcessor::processTasks() {
	while (!_abort) {
		SurfaceExtractionTask* task = nullptr;
		_pendingTasks.waitAndPop(task);
		if (task != nullptr) {
			task->process();
		}
	}
}
#endif
OctreeVolume::BackgroundTaskProcessor::BackgroundTaskProcessor(uint8_t noOfThreads) {
#if BACKGROUND_TASK_ARE_THREADED > 0
	for (uint8_t ct = 0; ct < noOfThreads; ++ct) {
		_threads.emplace_back(std::bind(&BackgroundTaskProcessor::processTasks, this));
	}
#endif
}

OctreeVolume::BackgroundTaskProcessor::~BackgroundTaskProcessor() {
#if BACKGROUND_TASK_ARE_THREADED > 0
	_abort = true;
	_pendingTasks.abortWait();
	for (auto i = _threads.begin(); i != _threads.end(); ++i) {
		i->join();
	}
#endif
}

void OctreeVolume::BackgroundTaskProcessor::addTask(SurfaceExtractionTask* task) {
#if BACKGROUND_TASK_ARE_THREADED > 0
	_pendingTasks.push(task);
#else
	task->process();
#endif
}

OctreeVolume::OctreeVolume(PagedVolume* volume, const Region& region, uint32_t baseNodeSize) :
		_region(region), _volume(volume), _octree(this, baseNodeSize) {
}

void OctreeVolume::setVoxel(int32_t x, int32_t y, int32_t z, const Voxel& value, bool markAsModified) {
	core_assert_msg(getRegion().containsPoint(x, y, z), "Attempted to write to a voxel which is outside of the volume");
	pagedVolume()->setVoxel(x, y, z, value);
	if (markAsModified) {
		octree().markDataAsModified(x, y, z, octree().time());
	}
}

}
