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
	core::StringMap<voxel::RawVolume*> _volumes core_thread_guarded_by(_mutex);
	core_trace_mutex(core::Lock, _mutex, "VolumeCache");
public:
	~VolumeCache();
	/**
	 * The returned volume is not owned by the caller. The cache will delete the memory.
	 */
	voxel::RawVolume* loadVolume(const core::String &fullPath);
	/**
	 * Remove the volume with the given path from the cache - and free the memory of the volume.
	 *
	 * @sa removeVolume(path)
	 */
	bool deleteVolume(const char* fullPath);
	/**
	 * Remove the volume with the given path from the cache - but don't delete it here.
	 * It is the caller's responsibily to properly delete the volume pointer.
	 *
	 * @sa deleteVolume(path)
	 */
	bool removeVolume(const char* fullPath);

	bool init() override;
	void shutdown() override;
	void construct() override;
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
