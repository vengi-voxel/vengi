/**
 * @file
 */

#pragma once

#include "MeshExporter.h"

namespace tinyobj {
struct mesh_t;
struct attrib_t;
struct material_t;
}

namespace voxel {
/**
 * @brief Wavefront Object
 */
class OBJFormat : public MeshExporter {
private:
	bool writeMtlFile(const core::String& mtlName, const core::String &paletteName) const;
	static void calculateAABB(const tinyobj::mesh_t &mesh, const tinyobj::attrib_t &attrib, glm::vec3 &mins,
							  glm::vec3 &maxs);
	static void subdivideShape(const tinyobj::mesh_t &mesh, const core::StringMap<image::ImagePtr> &textures,
							  const tinyobj::attrib_t &attrib, const std::vector<tinyobj::material_t> &materials,
							  core::DynamicArray<Tri> &subdivided);

public:
	bool saveMeshes(const Meshes& meshes, const core::String &filename, io::SeekableWriteStream& stream, const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;
	/**
	 * @brief Voxelizes the input mesh
	 */
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
};
}
