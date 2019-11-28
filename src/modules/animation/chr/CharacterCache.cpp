/**
 * @file
 */

#include "CharacterCache.h"
#include "CharacterSkeleton.h"
#include "core/io/Filesystem.h"
#include "core/App.h"
#include "core/Common.h"
#include "voxelformat/Loader.h"
#include "voxelformat/VoxFileFormat.h"

namespace animation {

bool CharacterCache::loadGlider(const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]) {
	const char *fullPath = "models/glider.vox";
	voxel::Mesh& mesh = cacheEntry(fullPath);
	if (mesh.getNoOfVertices() > 0) {
		meshes[std::enum_value(CharacterMeshType::Glider)] = &mesh;
		return true;
	}
	if (loadMesh(fullPath, mesh)) {
		meshes[std::enum_value(CharacterMeshType::Glider)] = &mesh;
		return true;
	}
	meshes[std::enum_value(CharacterMeshType::Glider)] = nullptr;
	Log::error("Failed to load glider");
	return false;
}

bool CharacterCache::getCharacterModel(const CharacterSettings& settings, Vertices& vertices, Indices& indices) {
	const voxel::Mesh* meshes[std::enum_value(CharacterMeshType::Max)] {};
	getMeshes(settings, meshes, [&] () {
		return loadGlider(meshes);
	});

	vertices.clear();
	indices.clear();
	vertices.reserve(3000);
	indices.reserve(5000);
	IndexType indexOffset = (IndexType)0;
	int meshCount = 0;
	// merge everything into one buffer
	for (int i = 0; i < std::enum_value(CharacterMeshType::Max); ++i) {
		const voxel::Mesh *mesh = meshes[i];
		if (mesh == nullptr) {
			continue;
		}
		const BoneIds& bids = settings.boneIds(i);
		core_assert_msg(bids.num >= 0 && bids.num <= 2,
				"number of bone ids is invalid: %i (for mesh type %i)q",
				(int)bids.num, i);
		for (uint8_t b = 0u; b < bids.num; ++b) {
			const uint8_t boneId = bids.bones[b];
			const std::vector<voxel::VoxelVertex>& meshVertices = mesh->getVertexVector();
			for (const voxel::VoxelVertex& v : meshVertices) {
				vertices.emplace_back(Vertex{v.position, v.colorIndex, boneId, v.ambientOcclusion});
			}

			const std::vector<voxel::IndexType>& meshIndices = mesh->getIndexVector();
			if (bids.mirrored[b]) {
				// if a model is mirrored, this is usually acchieved with negative scaling values
				// thus we have to reverse the winding order here to make the face culling work again
				for (auto i = meshIndices.rbegin(); i != meshIndices.rend(); ++i) {
					indices.push_back((IndexType)(*i) + indexOffset);
				}
			} else {
				for (voxel::IndexType idx : meshIndices) {
					indices.push_back((IndexType)idx + indexOffset);
				}
			}
			indexOffset = (IndexType)vertices.size();
		}
		++meshCount;
	}
	indices.resize(indices.size());
	core_assert(indices.size() % 3 == 0);

	return true;
}

bool CharacterCache::getItemModel(const char *itemName, Vertices& vertices, Indices& indices) {
	char fullPath[128];
	if (!core::string::formatBuf(fullPath, sizeof(fullPath), "models/items/%s.vox", itemName)) {
		Log::error("Failed to initialize the item path buffer. Can't load item %s.", itemName);
		return false;
	}
	return getModel(fullPath, BoneId::Tool, vertices, indices);
}

}
