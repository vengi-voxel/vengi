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

/**
 * @return The used bones for a given mesh type.
 * @note If you have mirrored bones like right, left hand, make sure that you set the mirrored
 * boolean properly. Otherwise the winding order is not fixed while assembling the ibo
 */
static inline BoneIds boneIds(CharacterMeshType type) {
	BoneIds b;
	switch (type) {
	case CharacterMeshType::Head:
		b.num = 1;
		b.bones[0] = std::enum_value(BoneId::Head);
		break;
	case CharacterMeshType::Chest:
		b.num = 1;
		b.bones[0] = std::enum_value(BoneId::Chest);
		break;
	case CharacterMeshType::Belt:
		b.num = 1;
		b.bones[0] = std::enum_value(BoneId::Belt);
		break;
	case CharacterMeshType::Pants:
		b.num = 1;
		b.bones[0] = std::enum_value(BoneId::Pants);
		break;
	case CharacterMeshType::Hand:
		b.num = 2;
		b.bones[0] = std::enum_value(BoneId::RightHand);
		b.bones[1] = std::enum_value(BoneId::LeftHand);
		b.mirrored[1] = true;
		break;
	case CharacterMeshType::Foot:
		b.num = 2;
		b.bones[0] = std::enum_value(BoneId::RightFoot);
		b.bones[1] = std::enum_value(BoneId::LeftFoot);
		b.mirrored[1] = true;
		break;
	case CharacterMeshType::Shoulder:
		b.num = 2;
		b.bones[0] = std::enum_value(BoneId::RightShoulder);
		b.bones[1] = std::enum_value(BoneId::LeftShoulder);
		b.mirrored[1] = true;
		break;
	case CharacterMeshType::Glider:
		b.num = 1;
		b.bones[0] = std::enum_value(BoneId::Glider);
		break;
	case CharacterMeshType::Max:
		core_assert(false);
		break;
	}

	return b;
}

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
	return false;
}

bool CharacterCache::getCharacterMeshes(const CharacterSettings& settings, const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]) {
	for (size_t i = 0; i < settings.paths.size(); ++i) {
		if (settings.paths[i] == nullptr || settings.paths[i]->empty()) {
			meshes[i] = nullptr;
			continue;
		}
		const std::string& fullPath = settings.fullPath((CharacterMeshType)i);
		if (!load(fullPath, (CharacterMeshType)i, meshes)) {
			Log::error("Failed to load %s", fullPath.c_str());
			return false;
		}
	}
	if (!loadGlider(meshes)) {
		Log::error("Failed to load glider");
		return false;
	}
	return true;
}

bool CharacterCache::getCharacterModel(const CharacterSettings& settings, Vertices& vertices, Indices& indices) {
	const voxel::Mesh* meshes[std::enum_value(CharacterMeshType::Max)] {};
	if (!getCharacterMeshes(settings, meshes)) {
		return false;
	}
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
		const CharacterMeshType meshType = (CharacterMeshType)i;
		const BoneIds& bids = boneIds(meshType);
		for (uint8_t b = 0u; b < bids.num; ++b) {
			const uint8_t boneId = bids.bones[b];
			const std::vector<voxel::VoxelVertex>& meshVertices = mesh->getVertexVector();
			for (voxel::VoxelVertex v : meshVertices) {
				if (meshType == CharacterMeshType::Foot) {
					v.position.y -= settings.skeletonAttr.hipOffset;
				}
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

bool CharacterCache::load(const std::string& filename, CharacterMeshType meshType, const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]) {
	voxel::Mesh& mesh = cacheEntry(filename.c_str());
	if (mesh.getNoOfVertices() > 0) {
		meshes[std::enum_value(meshType)] = &mesh;
		return true;
	}
	if (loadMesh(filename.c_str(), mesh)) {
		meshes[std::enum_value(meshType)] = &mesh;
		return true;
	}
	meshes[std::enum_value(meshType)] = nullptr;
	return false;
}

bool CharacterCache::getItemModel(const char *itemName, Vertices& vertices, Indices& indices) {
	char fullPath[128];
	if (!core::string::formatBuf(fullPath, sizeof(fullPath), "models/items/%s.vox", itemName)) {
		Log::error("Failed to initialize the item path buffer. Can't load item %s.", itemName);
		return false;
	}

	voxel::Mesh& mesh = cacheEntry(fullPath);
	if (mesh.getNoOfVertices() <= 0) {
		if (!loadMesh(fullPath, mesh)) {
			return false;
		}
	}

	vertices.clear();
	indices.clear();

	constexpr uint8_t boneId = std::enum_value(BoneId::Tool);
	vertices.reserve(mesh.getNoOfVertices());
	for (const voxel::VoxelVertex& v : mesh.getVertexVector()) {
		vertices.emplace_back(Vertex{v.position, v.colorIndex, boneId, v.ambientOcclusion});
	}
	//vertices.resize(mesh.getNoOfVertices());

	indices.reserve(mesh.getNoOfIndices());
	for (voxel::IndexType idx : mesh.getIndexVector()) {
		indices.push_back((IndexType)idx);
	}
	//indices.resize(mesh.getNoOfIndices());

	return true;
}

}
