/**
 * @file
 */

#include "Fill.h"

namespace voxedit {
namespace tool {

bool aabb(voxel::RawVolumeWrapper& target, const glm::ivec3& mins, const glm::ivec3& maxs, const voxel::Voxel& voxel, ModifierType modifierType, voxel::Region* modifiedRegion) {
	const bool deleteVoxels = (modifierType & ModifierType::Delete) == ModifierType::Delete;
	const bool overwrite = (modifierType & ModifierType::Place) == ModifierType::Place && deleteVoxels;
	const bool update = (modifierType & ModifierType::Update) == ModifierType::Update;
	voxel::Voxel placeVoxel = voxel;
	if (!overwrite && deleteVoxels) {
		placeVoxel = voxel::createVoxel(voxel::VoxelType::Air, 0);
	}
	glm::ivec3 modifiedMins((std::numeric_limits<int>::max)());
	glm::ivec3 modifiedMaxs((std::numeric_limits<int>::min)());
	int cnt = 0;
	for (int32_t z = mins.z; z <= maxs.z; ++z) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			for (int32_t x = mins.x; x <= maxs.x; ++x) {
				bool place = overwrite || deleteVoxels;
				if (!place) {
					const bool empty = isAir(target.voxel(x, y, z).getMaterial());
					if (update) {
						place = !empty;
					} else {
						place = empty;
					}
					if (!place) {
						continue;
					}
				}
				if (!target.setVoxel(x, y, z, placeVoxel)) {
					continue;
				}
				++cnt;
				modifiedMins.x = core_min(modifiedMins.x, x);
				modifiedMins.y = core_min(modifiedMins.y, y);
				modifiedMins.z = core_min(modifiedMins.z, z);

				modifiedMaxs.x = core_max(modifiedMaxs.x, x);
				modifiedMaxs.y = core_max(modifiedMaxs.y, y);
				modifiedMaxs.z = core_max(modifiedMaxs.z, z);
			}
		}
	}
	if (cnt <= 0) {
		return false;
	}
	if (modifiedRegion != nullptr) {
		*modifiedRegion = voxel::Region(modifiedMins, modifiedMaxs);
	}
	return true;
}

}
}
