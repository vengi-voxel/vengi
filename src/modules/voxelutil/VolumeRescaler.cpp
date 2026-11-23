/**
 * @file
 */

#include "VolumeRescaler.h"
#include "app/App.h"
#include "app/ForParallel.h"
#include "core/ProgressScope.h"
#include "voxel/VolumeSamplerUtil.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxelutil {

[[nodiscard]] voxel::RawVolume *scaleUp(const voxel::RawVolume &sourceVolume) {
	const voxel::Region srcRegion = sourceVolume.region();
	const math::Vec3i &targetDimensions = srcRegion.getDimensionsInVoxels() * 2 - 1;
	const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensions);
	if (!app::App::getInstance()->hasEnoughMemory(voxel::RawVolume::size(destRegion))) {
		return nullptr;
	}

	static const math::Vec3i directions[8] = {math::Vec3i(0, 0, 0), math::Vec3i(1, 0, 0), math::Vec3i(0, 1, 0),
											 math::Vec3i(1, 1, 0), math::Vec3i(0, 0, 1), math::Vec3i(1, 0, 1),
											 math::Vec3i(0, 1, 1), math::Vec3i(1, 1, 1)};

	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
	const int depth = srcRegion.getDepthInVoxels();
	core::ParallelProgress progress(depth);
	app::for_parallel(0, depth, [&sourceVolume, destVolume, &progress](int start, int end) {
		voxel::RawVolume::Sampler sourceSampler(sourceVolume);
		const math::Vec3i &dim = sourceVolume.region().getDimensionsInVoxels();
		const math::Vec3i &mins = sourceVolume.region().getLowerCorner();
		sourceSampler.setPosition(mins.x, mins.y, mins.z + start);
		for (int32_t z = start; z < end; ++z) {
			voxel::RawVolume::Sampler sourceSampler2 = sourceSampler;
			for (int32_t y = 0; y < dim.y; ++y) {
				voxel::RawVolume::Sampler sourceSampler3 = sourceSampler2;
				for (int32_t x = 0; x < dim.x; ++x) {
					const voxel::Voxel &voxel = sourceSampler3.voxel();
					const math::Vec3i targetPos(mins.x + x * 2, mins.y + y * 2, mins.z + z * 2);
					for (int i = 0; i < 8; ++i) {
						destVolume->setVoxel(targetPos + directions[i], voxel);
					}
					sourceSampler3.movePositiveX();
				}
				sourceSampler2.movePositiveY();
			}
			sourceSampler.movePositiveZ();
		}
		progress.add(end - start);
	});
	return destVolume;
}

[[nodiscard]] voxel::RawVolume *scaleVolume(const voxel::RawVolume *srcVolume, const math::Vec3f &scale,
										   const math::Vec3f &normalizedPivot, voxel::VoxelSampling sampling) {
	if (srcVolume == nullptr) {
		return nullptr;
	}

	const voxel::Region &srcRegion = srcVolume->region();
	const math::Vec3f srcDims(srcRegion.getDimensionsInVoxels());
	const math::Vec3f srcMinsF(srcRegion.getLowerCorner());

	// Compute the pivot point in source space
	const math::Vec3f srcPivot = srcMinsF + normalizedPivot * srcDims;

	// Use 0.5 offsets to represent voxel extents (each voxel is centered at its integer coordinate)
	// This is consistent with Region::rotate
	const math::Vec3f srcMinsEdge = srcMinsF - 0.5f;
	const math::Vec3f srcMaxsEdge = math::Vec3f(srcRegion.getUpperCorner()) + 0.5f;

	// Scale the edges relative to the pivot
	const math::Vec3f destMinsEdge = srcPivot + (srcMinsEdge - srcPivot) * scale;
	const math::Vec3f destMaxsEdge = srcPivot + (srcMaxsEdge - srcPivot) * scale;

	// Convert back to integer voxel coordinates
	const math::Vec3i destMins(math::floor(math::min(destMinsEdge, destMaxsEdge) + 0.5f));
	const math::Vec3i destMaxs(math::Vec3i(math::floor(math::max(destMinsEdge, destMaxsEdge) + 0.5f)) - 1);

	const voxel::Region destRegion(destMins, destMaxs);
	if (!app::App::getInstance()->hasEnoughMemory(voxel::RawVolume::size(destRegion))) {
		return nullptr;
	}

	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);

	// Inverse scale for backward mapping
	const math::Vec3f invScale = math::Vec3f(1.0f) / scale;

	// Backward mapping: iterate over destination and sample from source
	// For scaling around pivot: srcPos = pivot + (destPos - pivot) * invScale
	const int destDepth = destMaxs.z + 1 - destMins.z;
	core::ParallelProgress progress(destDepth);
	app::for_parallel(destMins.z, destMaxs.z + 1, [&](int start, int end) {
		for (int32_t z = start; z < end; ++z) {
			for (int32_t y = destMins.y; y <= destMaxs.y; ++y) {
				for (int32_t x = destMins.x; x <= destMaxs.x; ++x) {
					// Transform destination coordinate back to source space relative to pivot
					const math::Vec3f destPos(x, y, z);
					const math::Vec3f srcPos = srcPivot + (destPos - srcPivot) * invScale;

					voxel::RawVolume::Sampler srcSampler(srcVolume);
					voxel::Voxel voxel = voxel::sampleVoxel(srcSampler, sampling, srcPos);
					if (!voxel::isAir(voxel.getMaterial())) {
						destVolume->setVoxel(x, y, z, voxel);
					}
				}
			}
		}
		progress.add(end - start);
	});

	return destVolume;
}

} // namespace voxelutil
