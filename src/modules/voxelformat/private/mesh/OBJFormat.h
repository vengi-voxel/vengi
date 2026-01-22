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

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Wavefront Object", "text/plain", {"obj"}, {}, VOX_FORMAT_FLAG_MESH | FORMAT_FLAG_SAVE};
		return f;
	}
};
} // namespace voxelformat
