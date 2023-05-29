/**
 * @file
 */

#include "VolumeRotator.h"
#include "core/Assert.h"
#include "core/GLM.h"
#include "core/Log.h"
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
	return destVolume;
}

voxel::RawVolume *rotateAxis(const voxel::RawVolume *srcVolume, math::Axis axis) {
	const voxel::Region &srcRegion = srcVolume->region();
	const glm::ivec3 srcMins = srcRegion.getLowerCorner();
	const glm::ivec3 srcMaxs = srcRegion.getUpperCorner();
	if (axis == math::Axis::X) {
		const voxel::Region destRegion(srcMins.x, srcMins.z, srcMins.y, srcMaxs.x, srcMaxs.z, srcMaxs.y);
		voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);

		voxel::RawVolume::Sampler srcSampler(srcVolume);
		srcSampler.setPosition(srcRegion.getLowerCorner());
		for (int32_t z = srcRegion.getLowerZ(); z <= srcRegion.getUpperZ(); ++z) {
			voxel::RawVolume::Sampler srcSampler2 = srcSampler;
			for (int32_t y = 0; y < srcRegion.getHeightInVoxels(); ++y) {
				voxel::RawVolume::Sampler srcSampler3 = srcSampler2;
				for (int32_t x = srcRegion.getLowerX(); x <= srcRegion.getUpperX(); ++x) {
					const voxel::Voxel voxel = srcSampler3.voxel();
					if (!voxel::isAir(voxel.getMaterial())) {
						destVolume->setVoxel(x, z, srcMaxs.y - y, voxel);
					}
					srcSampler3.movePositiveX();
				}
				srcSampler2.movePositiveY();
			}
			srcSampler.movePositiveZ();
		}
		return destVolume;
	} else if (axis == math::Axis::Y) {
		const voxel::Region destRegion(srcMins.z, srcMins.y, srcMins.x, srcMaxs.z, srcMaxs.y, srcMaxs.x);
		voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);

		voxel::RawVolume::Sampler srcSampler(srcVolume);
		srcSampler.setPosition(srcRegion.getLowerCorner());
		for (int32_t z = 0; z < srcRegion.getDepthInVoxels(); ++z) {
			voxel::RawVolume::Sampler srcSampler2 = srcSampler;
			for (int32_t y = srcRegion.getLowerY(); y <= srcRegion.getUpperY(); ++y) {
				voxel::RawVolume::Sampler srcSampler3 = srcSampler2;
				for (int32_t x = srcRegion.getLowerX(); x <= srcRegion.getUpperX(); ++x) {
					const voxel::Voxel voxel = srcSampler3.voxel();
					if (!voxel::isAir(voxel.getMaterial())) {
						destVolume->setVoxel(srcMaxs.z - z, y, x, voxel);
					}
					srcSampler3.movePositiveX();
				}
				srcSampler2.movePositiveY();
			}
			srcSampler.movePositiveZ();
		}
		return destVolume;
	}
	const voxel::Region destRegion(srcMins.y, srcMins.x, srcMins.z, srcMaxs.y, srcMaxs.x, srcMaxs.z);
	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);

	voxel::RawVolume::Sampler srcSampler(srcVolume);
	srcSampler.setPosition(srcRegion.getLowerCorner());
	for (int32_t z = srcRegion.getLowerZ(); z <= srcRegion.getUpperZ(); ++z) {
		voxel::RawVolume::Sampler srcSampler2 = srcSampler;
		for (int32_t y = srcRegion.getLowerY(); y <= srcRegion.getUpperY(); ++y) {
			voxel::RawVolume::Sampler srcSampler3 = srcSampler2;
			for (int32_t x = 0; x < srcRegion.getWidthInVoxels(); ++x) {
				const voxel::Voxel voxel = srcSampler3.voxel();
				if (!voxel::isAir(voxel.getMaterial())) {
					destVolume->setVoxel(y, srcMaxs.x - x, z, voxel);
				}
				srcSampler3.movePositiveX();
			}
			srcSampler2.movePositiveY();
		}
		srcSampler.movePositiveZ();
	}
	return destVolume;
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
