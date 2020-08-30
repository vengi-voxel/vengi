/**
 * @file
 */

#include "WorldMgr.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "io/File.h"
#include "math/Random.h"
#include "core/concurrent/Concurrency.h"
#include "voxelutil/FloorTrace.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace voxelworld {

WorldMgr::WorldMgr(const voxel::PagedVolume::PagerPtr& pager) :
		_pager(pager), _random(_seed) {
}

WorldMgr::~WorldMgr() {
	shutdown();
}

glm::ivec3 WorldMgr::randomPos() const {
	int lowestX = -100;
	int lowestZ = -100;
	int highestX = 100;
	int highestZ = 100;
	const int x = _random.random(lowestX, highestX);
	const int z = _random.random(lowestZ, highestZ);
	const voxelutil::FloorTraceResult& trace = findWalkableFloor(glm::ivec3(x, voxel::MAX_HEIGHT / 2, z));
	return glm::ivec3(x, trace.heightLevel, z);
}

void WorldMgr::reset() {
	_volumeData->flushAll();
}

void WorldMgr::setSeed(unsigned int seed) {
	Log::info("Seed is: %u", seed);
	_seed = seed;
	_random.setSeed(seed);
}

voxel::PagedVolume::Sampler WorldMgr::sampler() {
	core_assert(_volumeData != nullptr);
	return voxel::PagedVolume::Sampler(_volumeData);
}

bool WorldMgr::init(uint32_t volumeMemoryMegaBytes, uint16_t chunkSideLength) {
	_volumeData = new voxel::PagedVolume(_pager.get(), volumeMemoryMegaBytes * 1024 * 1024, chunkSideLength);
	return true;
}

void WorldMgr::shutdown() {
	delete _volumeData;
	_volumeData = nullptr;
}

voxelutil::FloorTraceResult WorldMgr::findWalkableFloor(const glm::ivec3& position, int maxDistanceUpwards) const {
	core_assert_msg(_volumeData != nullptr, "WorldMgr is not initialized");
	voxel::PagedVolume::Sampler sampler(_volumeData);
	return voxelutil::findWalkableFloor(&sampler, position, maxDistanceUpwards);
}

}
