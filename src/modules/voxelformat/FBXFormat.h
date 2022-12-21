/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "core/collection/StringMap.h"

struct ufbx_node;
struct ufbx_scene;

namespace voxelformat {

/**
 * @brief Autodesk FBX
 * https://banexdevblog.wordpress.com/2014/06/23/a-quick-tutorial-about-the-fbx-ascii-format/
 * https://code.blender.org/2013/08/fbx-binary-file-format-specification/
 * https://github.com/libgdx/fbx-conv/
 *
 * @ingroup Formats
 */
class FBXFormat : public MeshFormat {
private:
	bool saveMeshesBinary(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
						  const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords, const SceneGraph &sceneGraph);
	bool saveMeshesAscii(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
						 const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords, const SceneGraph &sceneGraph);
	bool voxelizeGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	int addNode_r(const ufbx_scene *scene, const ufbx_node *node, const core::String &filename, SceneGraph &sceneGraph, const core::StringMap<image::ImagePtr> &textures, int parent) const;
	int addMeshNode(const ufbx_scene *scene, const ufbx_node *node, const core::String &filename, SceneGraph &sceneGraph, const core::StringMap<image::ImagePtr> &textures, int parent) const;
	int addCameraNode(const ufbx_scene *scene, const ufbx_node *node, SceneGraph &sceneGraph, int parent) const;

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const SceneGraph &sceneGraph, const Meshes &meshes,
					const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override;
};

} // namespace voxelformat
