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
	bool writeVertex(io::SeekableWriteStream &stream, const MeshExt &meshExt, const voxel::VoxelVertex &v1,
					 const scenegraph::SceneGraphTransform &transform, const glm::vec3 &scale);

	bool parseBinary(io::SeekableReadStream &stream, TriCollection &tris);
	bool parseAscii(io::SeekableReadStream &stream, TriCollection &tris);

	bool voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;

public:
	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const Meshes &meshes,
					const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override;
};
} // namespace voxelformat
