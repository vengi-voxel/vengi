/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "voxel/RawVolume.h"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace voxelformat {

class VolumeCache : public core::IComponent {
private:
	std::unordered_map<std::string, voxel::RawVolume*> _volumes;
	std::mutex _mutex;
public:
	~VolumeCache();
	/**
	 * The returned volume is now owned by the caller. The cache will delete the memory.
	 */
	voxel::RawVolume* loadVolume(const char* fullPath);

	bool init() override;
	void shutdown() override;
	void construct() override;
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
