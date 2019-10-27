/**
 * @file
 */

#include "WorldMgr.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "core/io/File.h"
#include "math/Random.h"
#include "core/concurrent/Concurrency.h"
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
	const int y = findWalkableFloor(glm::ivec3(x, voxel::MAX_HEIGHT / 2, z));
	return glm::ivec3(x, y, z);
}

void WorldMgr::reset() {
	_volumeData->flushAll();
}

void WorldMgr::setSeed(unsigned int seed) {
	Log::info("Seed is: %u", seed);
	_seed = seed;
	_random.setSeed(seed);
}

bool WorldMgr::init(uint32_t volumeMemoryMegaBytes, uint16_t chunkSideLength) {
	_volumeData = new voxel::PagedVolume(_pager.get(), volumeMemoryMegaBytes * 1024 * 1024, chunkSideLength);
	return true;
}

void WorldMgr::shutdown() {
	delete _volumeData;
	_volumeData = nullptr;
}

int WorldMgr::findWalkableFloor(const glm::ivec3& position, int maxDistanceY) const {
	core_trace_scoped(FindWalkableFloor);
	voxel::PagedVolume::Sampler sampler(_volumeData);
	sampler.setPosition(position);
	if (!sampler.currentPositionValid()) {
		return voxel::NO_FLOOR_FOUND;
	}

	const voxel::VoxelType type = sampler.voxel().getMaterial();
	if (voxel::isEnterable(type)) {
		const int maxDistance = (glm::min)(maxDistanceY, position.y);
		for (int i = 0; i < maxDistance; ++i) {
			sampler.moveNegativeY();
			if (!sampler.currentPositionValid()) {
				break;
			}
			const voxel::VoxelType mat = sampler.voxel().getMaterial();
			if (!voxel::isEnterable(mat)) {
				return sampler.position().y + 1;
			}
		}
		return voxel::NO_FLOOR_FOUND;
	}

	const int maxDistance = (glm::min)(maxDistanceY, voxel::MAX_HEIGHT - position.y);
	for (int i = 0; i < maxDistance; ++i) {
		sampler.movePositiveY();
		if (!sampler.currentPositionValid()) {
			break;
		}
		const voxel::VoxelType mat = sampler.voxel().getMaterial();
		if (voxel::isEnterable(mat)) {
			return sampler.position().y;
		}
	}
	return voxel::NO_FLOOR_FOUND;
}

}
