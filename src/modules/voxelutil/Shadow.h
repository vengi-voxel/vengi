#include "app/Async.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/collection/Queue.h"
#include "palette/Palette.h"
#include "voxel/Connectivity.h"
#include "voxel/Region.h"
#include "voxel/VolumeData.h"
#include "voxel/Voxel.h"

namespace voxelutil {

// fill the LightVolume with setValue after the BFS is done
struct LightNode {
	glm::ivec3 pos;
	int value;
};

#define MAX_SHADOW 255

using LightVolume = voxel::VolumeData<uint8_t>;
using LightQueue = core::Queue<LightNode, 512>;

template<class VOLUME>
static void propagateSunlight(VOLUME &volume, LightQueue &lightQueue, LightVolume &lightVolume, uint8_t lightStep) {
	const voxel::Region &region = lightVolume.region();

	Log::debug("Starting sunlight propagation with %i entries in the queue", (int)lightQueue.size());

	while (!lightQueue.empty()) {
		LightNode node = lightQueue.front();
		lightQueue.pop();
		if (node.value <= 0) {
			continue;
		}

		uint8_t nextLight = node.value >= lightStep ? node.value - lightStep : 0u;
		for (const glm::ivec3 &dir : voxel::arrayPathfinderCorners) {
			const glm::ivec3 &nextPos = node.pos + dir;
			if (!region.containsPoint(nextPos)) {
				continue;
			}
			const voxel::Voxel &v = volume.voxel(nextPos);
			if (!voxel::isAir(v.getMaterial())) {
				continue;
			}
			if (lightVolume.value(nextPos) >= nextLight) {
				continue;
			}
			lightVolume.setValue(nextPos, nextLight);
			lightQueue.push({nextPos, nextLight});
		}
	}
}

template<class VOLUME>
void shadow(VOLUME &volume, const palette::Palette &palette, uint8_t lightStep = 8) {
	voxel::Region lightVolumeRegion = volume.region();
	lightVolumeRegion.grow(1);
	LightVolume lightVolume(lightVolumeRegion, 0);
	LightQueue lightQueue;
	lightQueue.reserve(lightVolumeRegion.getWidthInVoxels() * lightVolumeRegion.getDepthInVoxels());

	{
		Log::debug("Seeding top layer with sunlight");
		typename VOLUME::Sampler sampler(&volume);
		sampler.setPosition(lightVolumeRegion.getLowerX(), lightVolumeRegion.getUpperY(), lightVolumeRegion.getLowerZ());
		for (int z = lightVolumeRegion.getLowerZ(); z <= lightVolumeRegion.getUpperZ(); ++z) {
			typename VOLUME::Sampler sampler2 = sampler;
			for (int x = lightVolumeRegion.getLowerX(); x <= lightVolumeRegion.getUpperX(); ++x) {
				typename VOLUME::Sampler sampler3 = sampler2;
				for (int y = lightVolumeRegion.getUpperY(); y >= lightVolumeRegion.getLowerY(); --y) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!voxel::isAir(voxel.getMaterial())) {
						// Stop sunlight when we hit first solid voxel from top
						break;
					}
					lightVolume.setValue(x, y, z, MAX_SHADOW);
					lightQueue.push({glm::ivec3(x, y, z), MAX_SHADOW});
					sampler3.moveNegativeY();
				}
				sampler2.movePositiveX();
			}
			sampler.movePositiveZ();
		}
	}

	Log::debug("Propagating sunlight through the volume");
	propagateSunlight(volume, lightQueue, lightVolume, lightStep);

	Log::debug("Applying shadows to voxels based on propagated light");
	auto fn = [&volume, &lightVolume, &palette] (int start, int end) {
		typename VOLUME::Sampler sampler(&volume);
		const voxel::Region &region = volume.region();
		sampler.setPosition(region.getLowerX(), region.getLowerY(), start);
		for (int z = start; z < end; ++z) {
			typename VOLUME::Sampler sampler2 = sampler;
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				typename VOLUME::Sampler sampler3 = sampler2;
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (voxel::isAir(voxel.getMaterial())) {
						sampler3.movePositiveX();
						continue;
					}
					uint8_t maxLight = 0u;
					for (const glm::ivec3 &dir : voxel::arrayPathfinderCorners) {
						const glm::ivec3 neighborPos(x + dir.x, y + dir.y, z + dir.z);
						if (voxel::isAir(volume.voxel(neighborPos).getMaterial())) {
							maxLight = core_max(maxLight, lightVolume.value(neighborPos));
						}
					}
					// max lit, no change
					// or fully surrounded by solid voxels - no need to change
					if (maxLight == MAX_SHADOW || maxLight == 0) {
						sampler3.movePositiveX();
						continue;
					}
					const float shadowFactor = (float)maxLight / (float)MAX_SHADOW;
					color::RGBA color = color::Color::darker(palette.color(voxel.getColor()), shadowFactor);
					const int palIdx = palette.getClosestMatch(color, voxel.getColor(), color::Color::Distance::HSB);
					sampler3.setVoxel(voxel::createVoxel(palette, palIdx));
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	};
	app::for_parallel(volume.region().getLowerZ(), volume.region().getUpperZ() + 1, fn);
}

} // namespace voxelutil
