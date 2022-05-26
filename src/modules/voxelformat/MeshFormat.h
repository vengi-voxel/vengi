/**
 * @file
 */

#pragma once

#include "Format.h"
#include "private/Tri.h"
#include <glm/geometric.hpp>

namespace voxelformat {

/**
 * @brief Convert the volume data into a mesh
 */
class MeshFormat : public Format {
protected:
	struct MeshExt {
		MeshExt(voxel::Mesh *mesh, const SceneGraphNode &node, bool applyTransform);
		voxel::Mesh *mesh;
		core::String name;
		bool applyTransform = false;

		glm::vec3 size{0.0f};
		int nodeId = -1;
	};
	using Meshes = core::DynamicArray<MeshExt>;
	virtual bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const SceneGraph &sceneGraph,
							const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
							const glm::vec3 &scale = glm::vec3(1.0f), bool quad = false, bool withColor = true,
							bool withTexCoords = true) = 0;

	static MeshExt* getParent(const voxelformat::SceneGraph &sceneGraph, Meshes &meshes, int nodeId);

	static glm::vec3 getScale();

public:
	using TriCollection = core::DynamicArray<Tri, 512>;

	/**
	 * Subdivide until we brought the triangles down to the size of 1 or smaller
	 */
	static void subdivideTri(const Tri &tri, TriCollection &tinyTris);
	static void voxelizeTris(voxelformat::SceneGraphNode &node, const TriCollection &tinyTris);

	bool loadGroups(const core::String &filename, io::SeekableReadStream &file, SceneGraph &sceneGraph) override;
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream) override;
};

} // namespace voxel
