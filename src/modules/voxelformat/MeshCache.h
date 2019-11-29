/**
 * @file
 */

#pragma once

#include "voxel/Mesh.h"
#include "voxelformat/VoxFileFormat.h"
#include "core/IComponent.h"
#include "core/String.h"
#include <memory>
#include <unordered_map>

namespace voxelformat {

class MeshCache : public core::IComponent {
protected:
	std::unordered_map<std::string, voxel::Mesh*> _meshes;

public:
	voxel::Mesh& cacheEntry(const char *path);
	bool loadMesh(const char* fullPath, voxel::Mesh& mesh);
	bool putMesh(const char* fullPath, const voxel::Mesh& mesh);
	bool init() override;
	void shutdown() override;
};

using MeshCachePtr = std::shared_ptr<MeshCache>;

}
