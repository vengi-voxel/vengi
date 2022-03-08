/**
 * @file
 */

#pragma once

#include "MeshExporter.h"

namespace voxel {

class VoxelVertex;

/**
 * @brief Standard Triangle Language
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
 */
class STLFormat : public MeshExporter {
private:
	struct Face {
		glm::vec3 normal {};
		glm::vec3 tri[3] {};
		uint16_t attribute = 0;
	};

	static void calculateAABB(const core::DynamicArray<Face> &faces, glm::vec3 &mins, glm::vec3 &maxs);
	static void subdivideShape(const core::DynamicArray<Face> &faces, core::DynamicArray<Tri> &subdivided);

	bool writeVertex(io::SeekableWriteStream &stream, const MeshExt &meshExt, const voxel::VoxelVertex &v1, const glm::vec3 &offset, const glm::vec3 &scale);

	bool parseBinary(io::SeekableReadStream &stream, core::DynamicArray<Face> &faces);
	bool parseAscii(io::SeekableReadStream &stream, core::DynamicArray<Face> &faces);

public:
	bool saveMeshes(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;
	/**
	 * @brief Voxelizes the input mesh
	 */
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) override;
};
} // namespace voxel
