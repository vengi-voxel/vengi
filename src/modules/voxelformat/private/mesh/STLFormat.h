/**
 * @file
 */

#pragma once

#include "MeshFormat.h"

namespace voxel {
struct VoxelVertex;
}

namespace scenegraph {
class SceneGraphTransform;
}

namespace voxelformat {

/**
 * @brief Standard Triangle Language
 *
 * https://en.wikipedia.org/wiki/STL_(file_format)
 *
 * @p Binary
 * UINT8[80] – Header
 * UINT32 – Number of triangles
 * foreach triangle
 * REAL32[3] – Normal vector
 * REAL32[3] – Vertex 1
 * REAL32[3] – Vertex 2
 * REAL32[3] – Vertex 3
 * UINT16 – Attribute byte count
 * end
 *
 * @ingroup Formats
 */
class STLFormat : public MeshFormat {
private:
	bool writeVertex(io::SeekableWriteStream &stream, const ChunkMeshExt &meshExt, const voxel::VoxelVertex &v1,
					 const scenegraph::SceneGraphTransform &transform, const glm::vec3 &scale);

	bool parseBinary(io::SeekableReadStream &stream, Mesh &mesh);
	bool parseAscii(io::SeekableReadStream &stream, Mesh &mesh);

	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;

public:
	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &meshes,
					const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Standard Triangle Language", "", {"stl"}, {}, VOX_FORMAT_FLAG_MESH | FORMAT_FLAG_SAVE};
		return f;
	}
};
} // namespace voxelformat
