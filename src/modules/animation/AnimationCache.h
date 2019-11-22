/**
 * @file
 */

#pragma once

#include "voxelformat/MeshCache.h"
#include "AnimationSettings.h"
#include "core/Assert.h"
#include <string>

namespace animation {

/**
 * @brief Cache @c voxel::Mesh instances for @c AnimationEntity
 */
template<typename T>
class AnimationCache : public voxelformat::MeshCache {
protected:
	bool load(const std::string& filename, size_t meshIndex, const voxel::Mesh* (&meshes)[std::enum_value(T::Max)]) {
		voxel::Mesh& mesh = cacheEntry(filename.c_str());
		if (mesh.getNoOfVertices() > 0) {
			meshes[meshIndex] = &mesh;
			return true;
		}
		if (loadMesh(filename.c_str(), mesh)) {
			meshes[meshIndex] = &mesh;
			return true;
		}
		meshes[meshIndex] = nullptr;
		return false;
	}

	bool getMeshes(const AnimationSettings<T>& settings, const voxel::Mesh* (&meshes)[std::enum_value(T::Max)],
			std::function<bool()> loadAdditional = {}) {
		for (size_t i = 0; i < settings.paths.size(); ++i) {
			if (settings.paths[i].empty()) {
				meshes[i] = nullptr;
				continue;
			}
			const std::string& fullPath = settings.fullPath((T)i);
			if (!load(fullPath, i, meshes)) {
				Log::error("Failed to load %s", fullPath.c_str());
				return false;
			}
		}
		if (loadAdditional && !loadAdditional()) {
			return false;
		}
		return true;
	}

public:
	bool getModel(const char *fullPath, BoneId bid, Vertices& vertices, Indices& indices) {
		voxel::Mesh& mesh = cacheEntry(fullPath);
		if (mesh.getNoOfVertices() <= 0) {
			if (!loadMesh(fullPath, mesh)) {
				return false;
			}
		}

		vertices.clear();
		indices.clear();

		const uint8_t boneIdInt = std::enum_value(bid);
		vertices.reserve(mesh.getNoOfVertices());
		for (const voxel::VoxelVertex& v : mesh.getVertexVector()) {
			vertices.emplace_back(Vertex{v.position, v.colorIndex, boneIdInt, v.ambientOcclusion});
		}
		//vertices.resize(mesh.getNoOfVertices());

		indices.reserve(mesh.getNoOfIndices());
		for (voxel::IndexType idx : mesh.getIndexVector()) {
			indices.push_back((IndexType)idx);
		}
		//indices.resize(mesh.getNoOfIndices());

		return true;
	}
};

}
