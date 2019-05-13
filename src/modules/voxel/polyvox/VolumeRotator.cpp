/**
 * @file
 */

#include "VolumeRotator.h"
#include "RawVolume.h"
#include "math/AABB.h"
#include "core/GLM.h"

namespace voxel {

/**
 * @param[in] source The RawVolume to rotate
 * @param[in] angles The angles for the x, y and z axis given in degrees
 * @param[in] increaseSize If you rotate e.g. by 45 degree, the rotated volume
 * would have a bigger size as the source volume. You can define that you would
 * like to cut it to the source volume size with this flag.
 * @return A new RawVolume. It's the caller's responsibility to free this
 * memory.
 */
RawVolume* rotateVolume(const RawVolume* source, const glm::vec3& angles, const Voxel& empty, const glm::vec3& pivot, bool increaseSize) {
	const float pitch = glm::radians(angles.x);
	const float yaw = glm::radians(angles.y);
	const float roll = glm::radians(angles.z);
#if 1
	const glm::mat4& rot = glm::eulerAngleXYZ(pitch, yaw, roll);
#else
	const glm::quat& quat = glm::normalize(
			  glm::angleAxis(pitch, glm::right)
			* glm::angleAxis(yaw, glm::up)
			* glm::angleAxis(roll, glm::forward));
	const glm::mat4& rot = glm::mat4_cast(quat);
#endif
	const voxel::Region& srcRegion = source->region();
	voxel::Region destRegion;

	if (increaseSize) {
		const glm::vec3 rotated1 = glm::rotate(rot, srcRegion.getLowerCornerf() - pivot);
		const glm::vec3 rotated2 = glm::rotate(rot, srcRegion.getUpperCornerf() - pivot);
		const float epsilon = 0.00001f;
		const glm::vec3 minsf = glm::min(rotated1, rotated2) + pivot + epsilon;
		const glm::vec3 maxsf = glm::max(rotated1, rotated2) + pivot + epsilon;
		destRegion = voxel::Region(glm::ivec3(minsf), glm::ivec3(maxsf));
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
				if (v == empty) {
					continue;
				}
				const glm::vec3 pos(x - pivot.x, y - pivot.y, z - pivot.z);
				const glm::vec3 rotatedPos = glm::rotate(rot, pos);
				const glm::vec3 newPos = rotatedPos + pivot;
				const glm::ivec3 volumePos(newPos);
				if (!destRegion.containsPoint(volumePos)) {
					continue;
				}

				destSampler.setPosition(volumePos);
				if (destSampler.voxel() == empty) {
					destSampler.setVoxel(v);
				}
			}
		}
	}
	return destination;
}

}
