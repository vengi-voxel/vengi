/**
 * @file
 */

#include "VolumeRotator.h"
#include "core/Assert.h"
#include "core/GLM.h"
#include "math/AABB.h"
#include "math/Axis.h"
#include "math/Math.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelutil/VoxelUtil.h"
#include "voxel/Region.h"
#include "voxel/Palette.h"
#include "voxel/Voxel.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace voxelutil {

/**
 * @param[in] source The RawVolume to rotate
 * @param[in] angles The angles for the x, y and z axis given in degrees
 * @return A new RawVolume. It's the caller's responsibility to free this
 * memory.
 */
voxel::RawVolume *rotateVolume(const voxel::RawVolume *srcVolume, const glm::vec3 &angles,
							   const glm::vec3 &normalizedPivot) {
	const float pitch = glm::radians(angles.x);
	const float yaw = glm::radians(angles.y);
	const float roll = glm::radians(angles.z);
	const glm::mat4 &mat = glm::eulerAngleXYZ(pitch, yaw, roll);
	const voxel::Region srcRegion = srcVolume->region();

	const glm::vec3 pivot(normalizedPivot * glm::vec3(srcRegion.getDimensionsInVoxels()));
	const voxel::Region &destRegion = srcRegion.rotate(mat, pivot);
	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
	voxel::RawVolumeWrapper destVolumeWrapper(destVolume);

	voxel::RawVolume::Sampler srcSampler(srcVolume);
	srcSampler.setPosition(srcRegion.getLowerCorner());
	for (int32_t z = srcRegion.getLowerZ(); z <= srcRegion.getUpperZ(); ++z) {
		voxel::RawVolume::Sampler srcSampler2 = srcSampler;
		for (int32_t y = srcRegion.getLowerY(); y <= srcRegion.getUpperY(); ++y) {
			voxel::RawVolume::Sampler srcSampler3 = srcSampler2;
			for (int32_t x = srcRegion.getLowerX(); x <= srcRegion.getUpperX(); ++x) {
				const voxel::Voxel voxel = srcSampler3.voxel();
				if (!voxel::isAir(voxel.getMaterial())) {
					const glm::ivec3 srcPos(x, y, z);
					const glm::ivec3 &destPos = math::transform(mat, srcPos, pivot);
					destVolumeWrapper.setVoxel(destPos, voxel);
				}
				srcSampler3.movePositiveX();
			}
			srcSampler2.movePositiveY();
		}
		srcSampler.movePositiveZ();
	}
	// TODO: use the pivot luke
	const glm::ivec3 &delta = srcRegion.getCenter() - destVolume->region().getCenter();
	destVolume->translate(delta);
	return destVolume;
}

voxel::RawVolume *rotateAxis(const voxel::RawVolume *source, math::Axis axis,
									  const glm::vec3 &normalizedPivot) {
	glm::vec3 angles{0.0f};
	angles[math::getIndexForAxis(axis)] = 90.0f;
	return rotateVolume(source, angles, normalizedPivot);
}

voxel::RawVolume *mirrorAxis(const voxel::RawVolume *source, math::Axis axis) {
	const voxel::Region &srcRegion = source->region();
	voxel::RawVolume *destination = new voxel::RawVolume(source);
	voxel::RawVolume::Sampler destSampler(destination);
	voxel::RawVolume::Sampler srcSampler(source);

	const glm::ivec3 &mins = srcRegion.getLowerCorner();
	const glm::ivec3 &maxs = srcRegion.getUpperCorner();

	if (axis == math::Axis::X) {
		for (int32_t z = mins.z; z <= maxs.z; ++z) {
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
	} else if (axis == math::Axis::Y) {
		for (int32_t z = mins.z; z <= maxs.z; ++z) {
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
	} else if (axis == math::Axis::Z) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
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
	}
	return destination;
}

} // namespace voxelutil
