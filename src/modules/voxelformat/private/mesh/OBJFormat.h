/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "io/Stream.h"

namespace tinyobj {
struct mesh_t;
struct attrib_t;
struct material_t;
struct shape_t;
} // namespace tinyobj

namespace voxelformat {
/**
 * @brief Wavefront Object
 *
 * https://en.wikipedia.org/wiki/Wavefront_.obj_file
 *
 * https://paulbourke.net/dataformats/mtl/
 *
 * @ingroup Formats
 */
class OBJFormat : public MeshFormat {
private:
	bool writeMtlFile(io::SeekableWriteStream &stream, const core::String &mtlId, const core::String &mapKd) const;

	/**
	 * @brief State for streaming OBJ export - tracks running offsets across multiple saveSingleMesh calls
	 */
	struct StreamState {
		io::SeekableWriteStream *objStream = nullptr;
		io::SeekableWriteStream *mtlStream = nullptr;
		core::Map<uint64_t, int> paletteMaterialIndices;
		core::StringMap<int> writtenTextures;
		int idxOffset = 0;
		int texcoordOffset = 0;
		core::String currentMaterial;
	};

protected:
	void loadPointCloud(tinyobj::attrib_t &tinyAttrib, tinyobj::shape_t &tinyShape, PointCloud &pointCloud);
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	bool voxelizeMeshShape(const tinyobj::shape_t &tinyShape, const tinyobj::attrib_t &tinyAttrib,
						   const tinyobj::material_t *tinyMaterials, scenegraph::SceneGraph &sceneGraph,
						   MeshMaterialMap &meshMaterials, const MeshMaterialArray &meshMaterialArray) const;

public:
	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &meshes,
					const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override;

	/**
	 * @brief Write a single mesh to an already-open OBJ stream, updating running offsets
	 */
	bool saveSingleMesh(StreamState &state, const scenegraph::SceneGraph &sceneGraph, const ChunkMeshExt &meshExt,
						const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale,
						bool quad, bool withColor, bool withTexCoords);

	bool saveGroupsStreaming(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							 const io::ArchivePtr &archive, const SaveContext &saveCtx);

	bool save(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
			  const io::ArchivePtr &archive, const SaveContext &ctx) override;

	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
	bool savePointCloud(const scenegraph::SceneGraph &sceneGraph, const PointCloud &pointCloud,
						const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale,
						bool withColor) const override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Wavefront Object", "text/plain", {"obj"}, {}, VOX_FORMAT_FLAG_MESH | FORMAT_FLAG_SAVE};
		return f;
	}
};
} // namespace voxelformat
