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
	const int y = findFloor(x, z, voxel::isFloor);
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

int WorldMgr::findWalkableFloor(const glm::vec3& position, float maxDistanceY) const {
	const voxel::VoxelType type = material(position.x, position.y, position.z);
	int y = voxel::NO_FLOOR_FOUND;
	if (voxel::isEnterable(type)) {
		raycast(position, glm::down, (glm::min)(maxDistanceY, position.y), [&] (const voxel::PagedVolume::Sampler& sampler) {
			voxel::VoxelType mat = sampler.voxel().getMaterial();
			if (!voxel::isEnterable(mat)) {
				y = sampler.position().y + 1;
				return false;
			}
			return true;
		});
	} else {
		raycast(position, glm::up, (glm::min)(maxDistanceY, (float)voxel::MAX_HEIGHT - position.y), [&] (const voxel::PagedVolume::Sampler& sampler) {
			voxel::VoxelType mat = sampler.voxel().getMaterial();
			if (voxel::isEnterable(mat)) {
				y = sampler.position().y;
				return false;
			}
			return true;
		});
	}
	return y;
}

int WorldMgr::chunkSize() const {
	return _volumeData->chunkSideLength();
}

voxel::VoxelType WorldMgr::material(int x, int y, int z) const {
	const voxel::Voxel& voxel = _volumeData->voxel(x, y, z);
	return voxel.getMaterial();
}

}
