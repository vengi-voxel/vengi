/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/SharedPtr.h"
#include "voxelformat/VolumeCache.h"
#include "core/collection/StringMap.h"
#include <glm/fwd.hpp>

namespace voxelworldrender {

/**
 * @brief This cache is for volume models that are not part of the world volumes.
 * They can be used to only render them (e.g. plants) or to let the player interact
 * with them (e.g. a chest entity)
 * @sa voxelworld::TreeVolumeCache
 */
class AssetVolumeCache : public core::IComponent {
private:
	voxelformat::VolumeCachePtr _volumeCache;
	int _plantCount = 0;
public:
	AssetVolumeCache(const voxelformat::VolumeCachePtr& volumeCache);

	bool init() override;
	void shutdown() override;

	/**
	 * @brief Ensure that the same volume is returned for the same input parameters.
	 * @param[in] pos world position
	 * @return voxel::RawVolume or @c nullptr if no suitable plant was found.
	 * @note Plants are stored by index in @c models/plants/
	 */
	voxel::RawVolume* loadPlant(const glm::ivec3& pos);
};

typedef core::SharedPtr<AssetVolumeCache> AssetVolumeCachePtr;

}
