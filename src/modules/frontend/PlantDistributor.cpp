#include "PlantDistributor.h"
#include "core/Trace.h"
#include "voxel/World.h"

namespace frontend {

void distributePlants(const voxel::WorldPtr& world, const glm::ivec3& pos, std::vector<glm::vec3>& translations) {
	core_trace_scoped(WorldRendererDistributePlants);
	const int size = world->meshSize();
	core::Random random(pos.x);
	const voxel::BiomeManager& biomeMgr = world->biomeManager();
	std::vector<glm::vec2> positions;
	biomeMgr.getPlantPositions(voxel::Region(pos.x, 0, pos.z, pos.x + size - 1, 0, pos.z + size - 1), positions, random, 5);
	for (const glm::vec2& p : positions) {
		const int y = world->findFloor(p.x, p.y, voxel::isFloor);
		if (y == voxel::NO_FLOOR_FOUND) {
			continue;
		}
		const glm::ivec3 translation(p.x, y, p.y);
		translations.push_back(translation);
	}
}

}
