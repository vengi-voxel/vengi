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

namespace voxel {

static inline int divFloor(int x, int y) {
	const bool quotientNegative = x < 0;
	return x / y - (x % y != 0 && quotientNegative);
}

static glm::ivec3 calcTransform(const glm::quat& t, int x, int y, int z, const glm::ivec3& pivot) {
	const glm::ivec3 c = glm::ivec3(x * 2, y * 2, z * 2) - pivot;
	const glm::ivec3 pos = glm::ivec3(t * glm::vec3(c.x + 0.5f, c.y + 0.5f, c.z + 0.5f));
	const glm::ivec3 rotated(divFloor(pos.x, 2), divFloor(pos.y, 2), divFloor(pos.z, 2));
	return rotated;
}

static glm::ivec3 calcTransform(const glm::quat& t, const glm::ivec3 &pos, const glm::ivec3& pivot) {
	return calcTransform(t, pos.x, pos.y, pos.z, pivot);
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
RawVolume* rotateVolume(const RawVolume* source, const glm::vec3& angles, const glm::ivec3& pivot, bool increaseSize) {
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
		const glm::ivec3 rotated1 = calcTransform(quat, srcRegion.getLowerCorner(), pivot);
		const glm::ivec3 rotated2 = calcTransform(quat, srcRegion.getUpperCorner(), pivot);
		const glm::ivec3 mins = (glm::min)(rotated1, rotated2);
		const glm::ivec3 maxs = (glm::max)(rotated1, rotated2);
		destRegion = voxel::Region(mins, maxs);
	} else {
		destRegion = srcRegion;
	}
	voxel::RawVolume* destination = new RawVolume(destRegion);
	voxel::RawVolume::Sampler destSampler(destination);
	voxel::RawVolume::Sampler srcSampler(source);

	for (int32_t z = srcRegion.getLowerZ(); z <= srcRegion.getUpperZ(); ++z) {
		for (int32_t y = srcRegion.getLowerY(); y <= srcRegion.getUpperY(); ++y) {
			for (int32_t x = srcRegion.getLowerX(); x <= srcRegion.getUpperX(); ++x) {
				srcSampler.setPosition(x, y, z);
				const Voxel& v = srcSampler.voxel();
				const glm::ivec3 volumePos = calcTransform(quat, x, y, z, pivot);
				if (!destRegion.containsPoint(volumePos)) {
					continue;
				}

				destSampler.setPosition(volumePos);
				destSampler.setVoxel(v);
			}
		}
	}
	return destination;
}

RawVolume* rotateAxis(const RawVolume* source, math::Axis axis) {
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
	RawVolume* destination = new RawVolume(destRegion);
	RawVolume::Sampler destSampler(destination);
	RawVolume::Sampler srcSampler(source);

	for (int32_t z = srcRegion.getLowerZ(); z <= srcRegion.getUpperZ(); ++z) {
		for (int32_t y = srcRegion.getLowerY(); y <= srcRegion.getUpperY(); ++y) {
			for (int32_t x = srcRegion.getLowerX(); x <= srcRegion.getUpperX(); ++x) {
				srcSampler.setPosition(x, y, z);
				const Voxel& v = srcSampler.voxel();
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

RawVolume* mirrorAxis(const RawVolume* source, math::Axis axis) {
	const voxel::Region& srcRegion = source->region();
	RawVolume* destination = new RawVolume(source);
	RawVolume::Sampler destSampler(destination);
	RawVolume::Sampler srcSampler(source);

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
