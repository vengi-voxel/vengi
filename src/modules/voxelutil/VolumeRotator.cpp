/**
 * @file
 */

#include "VolumeRotator.h"
#include "app/Async.h"
#include "math/Axis.h"
#include "math/Math.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/euler_angles.hpp>

namespace voxelutil {

/**
 * @param[in] srcVolume The RawVolume to rotate
 * @param[in] angles The angles for the x, y and z axis given in degrees
 * @return A new RawVolume. It's the caller's responsibility to free this
 * memory.
 */
voxel::RawVolume *rotateVolume(const voxel::RawVolume *srcVolume, const glm::ivec3 &angles,
							   const glm::vec3 &normalizedPivot) {
	// TODO: implement sampling http://www.leptonica.org/rotation.html
	const float pitch = glm::radians((float)angles.x);
	const float yaw = glm::radians((float)angles.y);
	const float roll = glm::radians((float)angles.z);
	const glm::mat4 &mat = glm::eulerAngleXYZ(pitch, yaw, roll);
	const voxel::Region srcRegion = srcVolume->region();

	const glm::vec3 pivot(normalizedPivot * glm::vec3(srcRegion.getDimensionsInVoxels()));
	const voxel::Region &destRegion = srcRegion.rotate(mat, pivot);
	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
	voxel::RawVolumeWrapper destVolumeWrapper(destVolume);
	voxelutil::visitVolumeParallel(
		*srcVolume,
		[&destVolumeWrapper, &mat, &pivot](int32_t x, int32_t y, int32_t z, const voxel::Voxel &voxel) {
			const glm::ivec3 &destPos = math::transform(mat, glm::ivec3{x, y, z}, pivot);
			destVolumeWrapper.setVoxel(destPos, voxel);
		});
	return destVolume;
}

voxel::RawVolume *rotateAxis(const voxel::RawVolume *srcVolume, math::Axis axis) {
	const voxel::Region &srcRegion = srcVolume->region();
	const glm::ivec3 srcMins = srcRegion.getLowerCorner();
	const glm::ivec3 srcMaxs = srcRegion.getUpperCorner();
	if (axis == math::Axis::X) {
		const voxel::Region destRegion(srcMins.x, srcMins.z, srcMins.y, srcMaxs.x, srcMaxs.z, srcMaxs.y);
		voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
		visitVolumeParallel(*srcVolume, [destVolume, srcMins, srcMaxs](int x, int y, int z, const voxel::Voxel& voxel) {
			destVolume->setVoxel(x, z, srcMaxs.y - (y - srcMins.y), voxel);
		});
		return destVolume;
	} else if (axis == math::Axis::Y) {
		const voxel::Region destRegion(srcMins.z, srcMins.y, srcMins.x, srcMaxs.z, srcMaxs.y, srcMaxs.x);
		voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
		visitVolumeParallel(*srcVolume, [destVolume, srcMins, srcMaxs](int x, int y, int z, const voxel::Voxel& voxel) {
			destVolume->setVoxel(srcMaxs.z - (z - srcMins.z), y, x, voxel);
		});
		return destVolume;
	}
	const voxel::Region destRegion(srcMins.y, srcMins.x, srcMins.z, srcMaxs.y, srcMaxs.x, srcMaxs.z);
	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
	visitVolumeParallel(*srcVolume, [destVolume, srcMins, srcMaxs](int x, int y, int z, const voxel::Voxel& voxel) {
		destVolume->setVoxel(y, srcMaxs.x - (x - srcMins.x), z, voxel);
	});
	return destVolume;
}

voxel::RawVolume *mirrorAxis(const voxel::RawVolume *source, math::Axis axis) {
	voxel::RawVolume *destination = new voxel::RawVolume(source);

	const voxel::Region &srcRegion = source->region();
	const glm::ivec3 &mins = srcRegion.getLowerCorner();
	const glm::ivec3 &maxs = srcRegion.getUpperCorner();

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
