/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "voxel/RawVolume.h"
#include "core/collection/StringMap.h"
#include <memory>
#include "core/concurrent/Lock.h"
#include "core/Trace.h"

namespace voxelformat {

/**
 * @brief Caches @c voxel::RawVolume instances by their name
 * @note The cache is threadsafe
 * @sa MeshCache
 */
class VolumeCache : public core::IComponent {
private:
	core::StringMap<voxel::RawVolume*> _volumes;
	core_trace_mutex(core::Lock, _mutex, "VolumeCache");
public:
	~VolumeCache();
	/**
	 * The returned volume is not owned by the caller. The cache will delete the memory.
	 */
	voxel::RawVolume* loadVolume(const char* fullPath);
	bool removeVolume(const char* fullPath);

	bool init() override;
	void shutdown() override;
	void construct() override;
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
