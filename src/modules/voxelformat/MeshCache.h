/**
 * @file
 */

#pragma once

#include "voxel/Mesh.h"
#include "core/IComponent.h"
#include "core/StringUtil.h"
#include "core/collection/StringMap.h"
#include <memory>

namespace voxelformat {

/**
 * @brief Cache voxel::Mesh instances by their name
 */
class MeshCache : public core::IComponent {
protected:
	core::StringMap<voxel::Mesh*> _meshes;

public:
	voxel::Mesh& cacheEntry(const char *path);
	bool loadMesh(const char* fullPath, voxel::Mesh& mesh);
	bool putMesh(const char* fullPath, const voxel::Mesh& mesh);
	bool removeMesh(const char *fullPath);
	bool init() override;
	void shutdown() override;
};

using MeshCachePtr = std::shared_ptr<MeshCache>;

}
