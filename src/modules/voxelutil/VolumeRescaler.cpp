/**
 * @file
 */

#include "VolumeRescaler.h"
#include "app/Async.h"
#include "voxel/VolumeSampler.h"

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

[[nodiscard]] voxel::RawVolume *scaleVolume(const voxel::RawVolume *srcVolume, const glm::vec3 &scale,
										   const glm::vec3 &normalizedPivot) {
	if (srcVolume == nullptr) {
		return nullptr;
	}

	const voxel::Region &srcRegion = srcVolume->region();
	const glm::vec3 srcDims(srcRegion.getDimensionsInVoxels());
	const glm::vec3 srcMinsF(srcRegion.getLowerCorner());

	// Compute the pivot point in source space
	const glm::vec3 srcPivot = srcMinsF + normalizedPivot * srcDims;

	// Use 0.5 offsets to represent voxel extents (each voxel is centered at its integer coordinate)
	// This is consistent with Region::rotate
	const glm::vec3 srcMinsEdge = srcMinsF - 0.5f;
	const glm::vec3 srcMaxsEdge = glm::vec3(srcRegion.getUpperCorner()) + 0.5f;

	// Scale the edges relative to the pivot
	const glm::vec3 destMinsEdge = srcPivot + (srcMinsEdge - srcPivot) * scale;
	const glm::vec3 destMaxsEdge = srcPivot + (srcMaxsEdge - srcPivot) * scale;

	// Convert back to integer voxel coordinates
	const glm::ivec3 destMins(glm::floor(glm::min(destMinsEdge, destMaxsEdge) + 0.5f));
	const glm::ivec3 destMaxs(glm::ivec3(glm::floor(glm::max(destMinsEdge, destMaxsEdge) + 0.5f)) - 1);

	const voxel::Region destRegion(destMins, destMaxs);
	if (!app::App::getInstance()->hasEnoughMemory(voxel::RawVolume::size(destRegion))) {
		return nullptr;
	}

	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);

	// Inverse scale for backward mapping
	const glm::vec3 invScale = glm::vec3(1.0f) / scale;

	// Backward mapping: iterate over destination and sample from source
	// For scaling around pivot: srcPos = pivot + (destPos - pivot) * invScale
	app::for_parallel(destMins.z, destMaxs.z + 1, [&](int start, int end) {
		for (int32_t z = start; z < end; ++z) {
			for (int32_t y = destMins.y; y <= destMaxs.y; ++y) {
				for (int32_t x = destMins.x; x <= destMaxs.x; ++x) {
					// Transform destination coordinate back to source space relative to pivot
					const glm::vec3 destPos(x, y, z);
					const glm::vec3 srcPos = srcPivot + (destPos - srcPivot) * invScale;

					// Sample from source volume using trilinear interpolation
					voxel::RawVolume::Sampler srcSampler(srcVolume);
					const voxel::Voxel voxel = voxel::sampleTrilinear(srcSampler, srcPos);
					if (!voxel::isAir(voxel.getMaterial())) {
						destVolume->setVoxel(x, y, z, voxel);
					}
				}
			}
		}
	});

	return destVolume;
}

} // namespace voxelutil
