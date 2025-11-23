/**
 * @file
 */

#include "VolumeRescaler.h"

namespace voxelutil {

[[nodiscard]] voxel::RawVolume *scaleUp(const voxel::RawVolume &sourceVolume) {
	const voxel::Region srcRegion = sourceVolume.region();
	const glm::ivec3 &targetDimensions = srcRegion.getDimensionsInVoxels() * 2 - 1;
	const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensions);
	if (!app::App::getInstance()->hasEnoughMemory(voxel::RawVolume::size(destRegion))) {
		return nullptr;
	}

	static const glm::ivec3 directions[8] = {glm::ivec3(0, 0, 0), glm::ivec3(1, 0, 0), glm::ivec3(0, 1, 0),
											 glm::ivec3(1, 1, 0), glm::ivec3(0, 0, 1), glm::ivec3(1, 0, 1),
											 glm::ivec3(0, 1, 1), glm::ivec3(1, 1, 1)};

	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
	app::for_parallel(0, srcRegion.getDepthInVoxels(), [&sourceVolume, destVolume](int start, int end) {
		voxel::RawVolume::Sampler sourceSampler(sourceVolume);
		const glm::ivec3 &dim = sourceVolume.region().getDimensionsInVoxels();
		const glm::ivec3 &mins = sourceVolume.region().getLowerCorner();
		sourceSampler.setPosition(mins.x, mins.y, mins.z + start);
		for (int32_t z = start; z < end; ++z) {
			voxel::RawVolume::Sampler sourceSampler2 = sourceSampler;
			for (int32_t y = 0; y < dim.y; ++y) {
				voxel::RawVolume::Sampler sourceSampler3 = sourceSampler2;
				for (int32_t x = 0; x < dim.x; ++x) {
					const voxel::Voxel &voxel = sourceSampler3.voxel();
					const glm::ivec3 targetPos(mins.x + x * 2, mins.y + y * 2, mins.z + z * 2);
					for (int i = 0; i < 8; ++i) {
						destVolume->setVoxel(targetPos + directions[i], voxel);
					}
					sourceSampler3.movePositiveX();
				}
				sourceSampler2.movePositiveY();
			}
			sourceSampler.movePositiveZ();
		}
	});
	return destVolume;
}

} // namespace voxelutil
