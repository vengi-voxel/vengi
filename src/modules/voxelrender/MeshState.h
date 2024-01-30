/**
 * @file
 */

#include "core/collection/Array.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include <unordered_map>

#include "core/GLM.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/hash.hpp>

namespace voxelrender {

static constexpr int MAX_VOLUMES = 2048;

enum MeshType { MeshType_Opaque, MeshType_Transparency, MeshType_Max };

// TODO: extract the mesh creation from RawVolumeRenderer and use it here
// this would allow us to perform the loading of the scene graph in a separate thread and the mesh extraction too,
// before we have to hand over to the renderer thread
class MeshState {
public:
	struct ExtractionCtx {
		ExtractionCtx() {
		}
		ExtractionCtx(const glm::ivec3 &_mins, int _idx, voxel::ChunkMesh &&_mesh)
			: mins(_mins), idx(_idx), mesh(_mesh) {
		}
		glm::ivec3 mins{};
		int idx = -1;
		voxel::ChunkMesh mesh;

		inline bool operator<(const ExtractionCtx &rhs) const {
			return idx < rhs.idx;
		}
	};

	typedef core::Array<voxel::Mesh *, MAX_VOLUMES> Meshes;
	typedef std::unordered_map<glm::ivec3, Meshes> MeshesMap;
	MeshesMap _meshes[MeshType_Max];

	void setOpaque(const glm::ivec3 &pos, int idx, voxel::Mesh &&mesh);
	void setTransparent(const glm::ivec3 &pos, int idx, voxel::Mesh &&mesh);
	void set(ExtractionCtx &ctx);
	bool deleteMeshes(const glm::ivec3 &pos, int idx);
	bool deleteMeshes(int idx);

	void clear();
};

} // namespace voxelrender
