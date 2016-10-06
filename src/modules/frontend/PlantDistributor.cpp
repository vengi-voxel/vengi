#include "PlantDistributor.h"
#include "core/Trace.h"
#include "voxel/World.h"

namespace frontend {

void distributePlants(const voxel::WorldPtr& world, int amount, const glm::ivec3& pos, std::vector<glm::vec3>& translations) {
	core_trace_scoped(WorldRendererDistributePlants);
	core::Random random(world->seed() + pos.x + pos.y + pos.z);
	const int size = world->getMeshSize();
	const voxel::BiomeManager& biomeMgr = world->getBiomeManager();
	for (;;) {
		if (amount-- <= 0) {
			return;
		}
		const int lx = random.random(1, size - 1);
		const int nx = pos.x + lx;
		const int lz = random.random(1, size - 1);
		const int nz = pos.z + lz;
		const int y = world->findFloor(nx, nz, voxel::isFloor);
		if (y == -1) {
			continue;
		}
		const glm::ivec3 translation(nx, y, nz);
		if (!biomeMgr.hasPlants(translation)) {
			continue;
		}

		translations.push_back(translation);
		Log::trace("plant at %i:%i:%i (%i)", nx, y, nz, (int)translations.size());
	}
}

}
