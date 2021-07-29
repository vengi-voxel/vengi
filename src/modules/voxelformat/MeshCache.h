/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "voxel/Mesh.h"
#include "core/IComponent.h"
#include "core/StringUtil.h"
#include "core/collection/StringMap.h"

namespace voxelformat {

/**
 * @brief Cache @c voxel::Mesh instances by their name
 * @note The cache is @b not threadsafe
 * @sa MeshCache
 */
class MeshCache : public core::IComponent {
protected:
	core::StringMap<voxel::Mesh*> _meshes;
	int _initCalls = 0;

	voxel::Mesh& cacheEntry(const char *fullPath);
	bool loadMesh(const char* fullPath, voxel::Mesh& mesh);
public:
	~MeshCache();
	const voxel::Mesh* getMesh(const char *fullPath);
	bool removeMesh(const char *fullPath);
	bool init() override;
	void shutdown() override;
};

using MeshCachePtr = core::SharedPtr<MeshCache>;

}
