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
	std::unordered_map<core::String, voxel::RawVolume*, core::StringHash> _volumes;
	std::mutex _mutex;
public:
	~VolumeCache();
	/**
	 * The returned volume is not owned by the caller. The cache will delete the memory.
	 */
	voxel::RawVolume* loadVolume(const char* fullPath);

	bool init() override;
	void shutdown() override;
	void construct() override;
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
