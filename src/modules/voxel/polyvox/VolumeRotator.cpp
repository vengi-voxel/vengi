/**
 * @file
 */

#include "VolumeRotator.h"
#include "RawVolume.h"
#include "core/AABB.h"

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
RawVolume* rotateVolume(const RawVolume* source, const glm::vec3& angles, bool increaseSize) {
	const glm::quat& quat = glm::normalize(
			  glm::angleAxis(glm::radians(angles.x), glm::right)
			* glm::angleAxis(glm::radians(angles.y), glm::up)
			* glm::angleAxis(glm::radians(angles.z), glm::backward));
	const glm::mat4& rot = glm::mat4_cast(quat);

	const voxel::Region& srcRegion = source->getEnclosingRegion();
	const glm::ivec3& srcCenter = srcRegion.getCentre();
	voxel::Region destRegion;

	if (increaseSize) {
		const glm::ivec4 mins(srcRegion.getLowerCorner(), 1);
		const glm::ivec4 maxs(srcRegion.getUpperCorner(), 1);
		const glm::ivec4& newMins = rot * mins;
		const glm::ivec4& newMaxs = rot * maxs;
		const glm::ivec3 vertices[] = { newMins.xyz(), newMaxs.xyz() };
		core::AABB<int> aabb = core::AABB<int>::construct(vertices, SDL_arraysize(vertices));
		Log::info("1 angles(%s)", glm::to_string(angles).c_str());
		Log::info("1 newins(%s)/newmaxs(%s)", glm::to_string(newMins).c_str(), glm::to_string(newMaxs).c_str());
		Log::info("1 srcregion(%s/%s)", glm::to_string(srcRegion.getLowerCorner()).c_str(), glm::to_string(srcRegion.getUpperCorner()).c_str());
		Log::info("1 destregion(%s/%s)", glm::to_string(aabb.getLowerCorner()).c_str(), glm::to_string(aabb.getUpperCorner()).c_str());
		Log::info("3 center(%s)", glm::to_string(aabb.getCenter()).c_str());
		aabb.shift(-aabb.getLowerCorner());
		destRegion = voxel::Region(aabb.getLowerCorner(), aabb.getUpperCorner());
		Log::info("2 destregion(%s/%s)", glm::to_string(aabb.getLowerCorner()).c_str(), glm::to_string(aabb.getUpperCorner()).c_str());
	} else {
		destRegion = srcRegion;
	}
	voxel::RawVolume* destination = new RawVolume(destRegion);

	const int32_t depth = srcRegion.getDepthInVoxels();
	const int32_t height = srcRegion.getHeightInVoxels();
	const int32_t width = srcRegion.getWidthInVoxels();
	for (int32_t z = 0; z < depth; z++) {
		for (int32_t y = 0; y < height; y++) {
			for (int32_t x = 0; x < width; x++) {
				const Voxel& v = source->getVoxel(x, y, z);
				if (v.getMaterial() == VoxelType::Air) {
					continue;
				}
				const glm::ivec4 pos(x - srcCenter.x, y - srcCenter.y, z - srcCenter.z, 1);
				glm::ivec4 newPos = rot * pos;
				newPos.x += srcCenter.x;
				newPos.y += srcCenter.y;
				newPos.z += srcCenter.z;
				if (!destRegion.containsPoint(newPos.x, newPos.y, newPos.z)) {
					continue;
				}
				const glm::ivec3& volumePos = newPos.xyz();
				Log::info("pos(%i:%i:%i), volumePos(%i:%i:%i)", pos.x, pos.y, pos.z, volumePos.x, volumePos.y, volumePos.z);
				destination->setVoxel(volumePos, v);
			}
		}
	}
	return destination;
}

}
