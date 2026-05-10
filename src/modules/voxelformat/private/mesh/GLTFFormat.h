/**
 * @file
 */

#pragma once

#include "MeshFormat.h"

struct cgltf_data;
struct cgltf_node;
struct cgltf_material;
struct cgltf_primitive;

namespace voxelformat {

/**
 * @brief GL Transmission Format
 * https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/figures/gltfOverview-2.0.0b.png
 *
 * @li Viewer including animations: https://sandbox.babylonjs.com/
 * @li GLTF-Validator: https://github.khronos.org/glTF-Validator/
 * @li GLTF Extensions: https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos
 *
 * @ingroup Formats
 */
class GLTFFormat : public MeshFormat {
private:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	int addNode_r(const cgltf_data *data, const cgltf_node *node, const core::String &filename,
				  const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent,
				  core::Map<const cgltf_node *, int> &nodeMap) const;
	MeshMaterialPtr loadMaterial(const cgltf_data *data, const cgltf_material *material, const core::String &filename,
								 const io::ArchivePtr &archive) const;
	void importAnimations(const cgltf_data *data, scenegraph::SceneGraph &sceneGraph,
						  const core::Map<const cgltf_node *, int> &nodeMap) const;

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;
	bool savePointCloud(const scenegraph::SceneGraph &sceneGraph, const PointCloud &pointCloud,
						const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale,
						bool withColor) const override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"GL Transmission Format",
									   "", // model/gltf+json, model/gltf-binary
									   {"gltf", "glb", "vrm"},
									   {},
									   VOX_FORMAT_FLAG_MESH | VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
