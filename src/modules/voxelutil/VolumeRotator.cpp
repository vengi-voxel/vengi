/**
 * @file
 */

#include "VolumeRotator.h"
#include "app/ForParallel.h"
#include "math/Axis.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "math/Mat4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Mat4Ops.h"
#include "math/Scalar.h"

namespace voxelutil {

/**
 * @param[in] srcVolume The RawVolume to rotate
 * @param[in] mat The transformation matrix representing the rotation
 * @return A new RawVolume. It's the caller's responsibility to free this
 * memory.
 *
 * Uses inverse transformation (backward mapping) with trilinear sampling to avoid holes.
 */
voxel::RawVolume *rotateVolume(const voxel::RawVolume *srcVolume, const math::Mat4f &mat,
							   const math::Vec3f &normalizedPivot, voxel::VoxelSampling sampling) {
	const math::Mat4f &invMat = math::inverse(mat);
	const voxel::Region srcRegion = srcVolume->region();

	const math::Vec3f pivot(normalizedPivot * math::Vec3f(srcRegion.getDimensionsInVoxels()));
	const voxel::Region &destRegion = srcRegion.rotate(mat, pivot);
	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);

	// Backward mapping: iterate over destination and sample from source
	const math::Vec3i &destMins = destRegion.getLowerCorner();
	const math::Vec3i &destMaxs = destRegion.getUpperCorner();

	auto func = [&](int start, int end) {
		for (int32_t z = start; z < end; ++z) {
			for (int32_t y = destMins.y; y <= destMaxs.y; ++y) {
				for (int32_t x = destMins.x; x <= destMaxs.x; ++x) {
					// Transform destination coordinate back to source space using inverse transformation
					const math::Vec3f srcPos = math::transform(invMat, math::Vec3f(x, y, z), pivot);
					// Sample from source volume using generic trilinear interpolation via sampler
					voxel::RawVolume::Sampler srcSampler(srcVolume);
					const voxel::Voxel voxel = voxel::sampleVoxel(srcSampler, sampling, srcPos);
					if (!voxel::isAir(voxel.getMaterial())) {
						destVolume->setVoxel(x, y, z, voxel);
					}
				}
			}
		}
	};
	app::for_parallel(destMins.z, destMaxs.z + 1, func);

	return destVolume;
}

voxel::RawVolume *rotateVolumeDegrees(const voxel::RawVolume *srcVolume, const math::Vec3i &angles,
									  const math::Vec3f &normalizedPivot, voxel::VoxelSampling sampling) {
	const float pitch = math::radians((float)angles.x);
	const float yaw = math::radians((float)angles.y);
	const float roll = math::radians((float)angles.z);
	const math::Mat4f &mat = math::eulerAngleXYZ(pitch, yaw, roll);
	return rotateVolume(srcVolume, mat, normalizedPivot, sampling);
}

voxel::RawVolume *rotateAxis(const voxel::RawVolume *srcVolume, math::Axis axis) {
	const voxel::Region &srcRegion = srcVolume->region();
	const math::Vec3i srcMins = srcRegion.getLowerCorner();
	const math::Vec3i srcMaxs = srcRegion.getUpperCorner();
	if (axis == math::Axis::X) {
		const voxel::Region destRegion(srcMins.x, srcMins.z, srcMins.y, srcMaxs.x, srcMaxs.z, srcMaxs.y);
		voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
		auto func = [destVolume, srcMins, srcMaxs](int x, int y, int z, const voxel::Voxel &voxel) {
			destVolume->setVoxel(x, z, srcMaxs.y - (y - srcMins.y), voxel);
		};
		visitVolumeParallel(*srcVolume, func);
		return destVolume;
	} else if (axis == math::Axis::Y) {
		const voxel::Region destRegion(srcMins.z, srcMins.y, srcMins.x, srcMaxs.z, srcMaxs.y, srcMaxs.x);
		voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
		auto func = [destVolume, srcMins, srcMaxs](int x, int y, int z, const voxel::Voxel &voxel) {
			destVolume->setVoxel(srcMaxs.z - (z - srcMins.z), y, x, voxel);
		};
		visitVolumeParallel(*srcVolume, func);
		return destVolume;
	}
	const voxel::Region destRegion(srcMins.y, srcMins.x, srcMins.z, srcMaxs.y, srcMaxs.x, srcMaxs.z);
	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
	auto func = [destVolume, srcMins, srcMaxs](int x, int y, int z, const voxel::Voxel &voxel) {
		destVolume->setVoxel(y, srcMaxs.x - (x - srcMins.x), z, voxel);
	};
	visitVolumeParallel(*srcVolume, func);
	return destVolume;
}

voxel::RawVolume *mirrorAxis(const voxel::RawVolume *source, math::Axis axis) {
	voxel::RawVolume *destination = new voxel::RawVolume(source);

	const voxel::Region &srcRegion = source->region();
	const math::Vec3i &mins = srcRegion.getLowerCorner();
	const math::Vec3i &maxs = srcRegion.getUpperCorner();

	if (axis == math::Axis::X) {
		app::for_parallel(mins.z, maxs.z + 1, [&source, &destination, mins, maxs] (int start, int end) {
			voxel::RawVolume::Sampler destSampler(destination);
			voxel::RawVolume::Sampler srcSampler(source);
			for (int32_t z = start; z < end; ++z) {
				for (int32_t y = mins.y; y <= maxs.y; ++y) {
					srcSampler.setPosition(mins.x, y, z);
					destSampler.setPosition(maxs.x, y, z);
					for (int32_t x = mins.x; x <= maxs.x; ++x) {
						destSampler.setVoxel(srcSampler.voxel());
						srcSampler.movePositiveX();
						destSampler.moveNegativeX();
					}
				}
			}
		});
	} else if (axis == math::Axis::Y) {
		app::for_parallel(mins.z, maxs.z + 1, [&source, &destination, mins, maxs] (int start, int end) {
			voxel::RawVolume::Sampler destSampler(destination);
			voxel::RawVolume::Sampler srcSampler(source);
			for (int32_t z = start; z < end; ++z) {
				for (int32_t x = mins.x; x <= maxs.x; ++x) {
					srcSampler.setPosition(x, mins.y, z);
					destSampler.setPosition(x, maxs.y, z);
					for (int32_t y = mins.y; y <= maxs.y; ++y) {
						destSampler.setVoxel(srcSampler.voxel());
						srcSampler.movePositiveY();
						destSampler.moveNegativeY();
					}
				}
			}
		});
	} else if (axis == math::Axis::Z) {
		app::for_parallel(mins.y, maxs.y + 1, [&source, &destination, mins, maxs] (int start, int end) {
			voxel::RawVolume::Sampler destSampler(destination);
			voxel::RawVolume::Sampler srcSampler(source);
			for (int32_t y = start; y < end; ++y) {
				for (int32_t x = mins.x; x <= maxs.x; ++x) {
					srcSampler.setPosition(x, y, mins.z);
					destSampler.setPosition(x, y, maxs.z);
					for (int32_t z = mins.z; z <= maxs.z; ++z) {
						destSampler.setVoxel(srcSampler.voxel());
						srcSampler.movePositiveZ();
						destSampler.moveNegativeZ();
					}
				}
			}
		});
	}
	return destination;
}

} // namespace voxelutil
