#include "PlanetGenerator.h"
#include "noise/SphereNoise.h"

namespace voxel {

void createPlanet(GeneratorContext& ctx, const glm::vec3& center, const Voxel& voxel, float scale) {
#if 0
	for (int lon = -180; lon < 180; ++lon) {
		for (int lat = -90; lat <= 90; ++lat) {
			const float n = noise::sphereNoise(lon, lat);
			const double distance = n * scale;
			if (distance > radius) {
				continue;
			}
			const glm::ivec3 pos(center.x + x, center.y, center.z + z);
			ctx.setVoxel(pos, voxel);
		}
	}
#endif
}

}
