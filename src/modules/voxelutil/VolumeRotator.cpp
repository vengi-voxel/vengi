/**
 * @file
 */

#include "VolumeRotator.h"
#include "voxel/RawVolume.h"
#include "math/AABB.h"
#include "core/GLM.h"
#include "core/Assert.h"
#include "voxel/Region.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace voxelutil {

static glm::ivec3 calcTransform(const glm::quat &q, const glm::ivec3& pos, const glm::vec4& pivot) {
	return glm::floor(q * (glm::vec4((float)pos.x + 0.5f, (float)pos.y + 0.5f, (float)pos.z + 0.5f, 1.0f) - pivot));
}

static glm::ivec3 calcTransform(const glm::quat &q, int x, int y, int z, const glm::vec4& pivot) {
	return calcTransform(q, glm::ivec3(x, y, z), pivot);
}

/**
 * @param[in] source The RawVolume to rotate
 * @param[in] angles The angles for the x, y and z axis given in degrees
 * @param[in] increaseSize If you rotate e.g. by 45 degree, the rotated volume
 * would have a bigger size as the source volume. You can define that you would
 * like to cut it to the source volume size with this flag.
 * @return A new RawVolume. It's the caller's responsibility to free this
 * memory.
 */
voxel::RawVolume* rotateVolume(const voxel::RawVolume* source, const glm::vec3& angles, const glm::vec3& pivot, bool increaseSize) {
	const float pitch = glm::radians(angles.x);
	const float yaw = glm::radians(angles.y);
	const float roll = glm::radians(angles.z);
	const glm::quat& quat = glm::normalize(
			  glm::angleAxis(pitch, glm::right)
			* glm::angleAxis(yaw, glm::up)
			* glm::angleAxis(roll, glm::forward));
	const voxel::Region& srcRegion = source->region();
	voxel::Region destRegion;

	if (increaseSize) {
		const glm::ivec3 rotated1 = calcTransform(quat, srcRegion.getLowerCorner(), glm::vec4(pivot, 0.0f));
		const glm::ivec3 rotated2 = calcTransform(quat, srcRegion.getUpperCorner(), glm::vec4(pivot, 0.0f));
		const glm::ivec3 mins = (glm::min)(rotated1, rotated2);
		const glm::ivec3 maxs = (glm::max)(rotated1, rotated2);
		destRegion = voxel::Region(mins, maxs);
	} else {
		destRegion = srcRegion;
	}
	voxel::RawVolume* destination = new voxel::RawVolume(destRegion);
	voxel::RawVolume::Sampler destSampler(destination);
	voxel::RawVolume::Sampler srcSampler(source);

	for (int32_t z = srcRegion.getLowerZ(); z <= srcRegion.getUpperZ(); ++z) {
		for (int32_t y = srcRegion.getLowerY(); y <= srcRegion.getUpperY(); ++y) {
			for (int32_t x = srcRegion.getLowerX(); x <= srcRegion.getUpperX(); ++x) {
				const glm::ivec3 volumePos = calcTransform(quat, x, y, z, glm::vec4(pivot, 0.0f));
				if (!destRegion.containsPoint(volumePos)) {
					continue;
				}
				srcSampler.setPosition(x, y, z);
				destSampler.setPosition(volumePos);
				destSampler.setVoxel(srcSampler.voxel());
			}
		}
	}
	return destination;
}

voxel::RawVolume* rotateAxis(const voxel::RawVolume* source, math::Axis axis) {
	const voxel::Region& srcRegion = source->region();
	voxel::Region destRegion = srcRegion;
	if (axis == math::Axis::Y) {
		destRegion.setLowerX(srcRegion.getLowerZ());
		destRegion.setLowerZ(srcRegion.getLowerX());
		destRegion.setUpperX(srcRegion.getUpperZ());
		destRegion.setUpperZ(srcRegion.getUpperX());
	} else if (axis == math::Axis::X) {
		destRegion.setLowerY(srcRegion.getLowerZ());
		destRegion.setLowerZ(srcRegion.getLowerX());
		destRegion.setUpperY(srcRegion.getUpperZ());
		destRegion.setUpperZ(srcRegion.getUpperY());
	} else {
		destRegion.setLowerY(srcRegion.getLowerX());
		destRegion.setLowerX(srcRegion.getLowerY());
		destRegion.setUpperY(srcRegion.getUpperX());
		destRegion.setUpperX(srcRegion.getUpperY());
	}
	core_assert(destRegion.isValid());
	voxel::RawVolume* destination = new voxel::RawVolume(destRegion);
	voxel::RawVolume::Sampler destSampler(destination);
	voxel::RawVolume::Sampler srcSampler(source);

	for (int32_t z = srcRegion.getLowerZ(); z <= srcRegion.getUpperZ(); ++z) {
		for (int32_t y = srcRegion.getLowerY(); y <= srcRegion.getUpperY(); ++y) {
			for (int32_t x = srcRegion.getLowerX(); x <= srcRegion.getUpperX(); ++x) {
				srcSampler.setPosition(x, y, z);
				const voxel::Voxel& v = srcSampler.voxel();
				glm::ivec3 pos(x, y, z);
				if (axis == math::Axis::X) {
					const int tmp = pos.y;
					pos.y = pos.z;
					pos.z = tmp;
				} else if (axis == math::Axis::Y) {
					const int tmp = pos.x;
					pos.x = pos.z;
					pos.z = tmp;
				} else {
					const int tmp = pos.x;
					pos.x = pos.y;
					pos.y = tmp;
				}
				core_assert_always(destSampler.setPosition(pos));
				core_assert_always(destSampler.setVoxel(v));
			}
		}
	}
	return destination;
}

voxel::RawVolume* mirrorAxis(const voxel::RawVolume* source, math::Axis axis) {
	const voxel::Region& srcRegion = source->region();
	voxel::RawVolume* destination = new voxel::RawVolume(source);
	voxel::RawVolume::Sampler destSampler(destination);
	voxel::RawVolume::Sampler srcSampler(source);

	const glm::ivec3& mins = srcRegion.getLowerCorner();
	const glm::ivec3& maxs = srcRegion.getUpperCorner();

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

}
