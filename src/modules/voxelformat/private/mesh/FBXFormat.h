/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "scenegraph/SceneGraphTransform.h"

struct ufbx_node;
struct ufbx_scene;

namespace voxelformat {

/**
 * @brief Autodesk FBX
 * https://banexdevblog.wordpress.com/2014/06/23/a-quick-tutorial-about-the-fbx-ascii-format/
 * https://code.blender.org/2013/08/fbx-binary-file-format-specification/
 * https://github.com/libgdx/fbx-conv/
 * https://github.com/BobbyAnguelov/FbxFormatConverter/releases/tag/v0.3
 *
 * @ingroup Formats
 */
class FBXFormat : public MeshFormat {
private:
	bool saveMeshesBinary(const ChunkMeshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
						  const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords,
						  const scenegraph::SceneGraph &sceneGraph);
	void writeTransformToProperties(io::SeekableWriteStream &stream, const scenegraph::SceneGraphTransform &transform);
	bool saveMeshesAscii(const ChunkMeshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
						 const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords,
						 const scenegraph::SceneGraph &sceneGraph);
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	int addNode_r(const ufbx_scene *scene, const ufbx_node *node, const core::String &filename,
				  const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent) const;
	int addMeshNode(const ufbx_scene *scene, const ufbx_node *node, const core::String &filename,
					const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent) const;
	int addCameraNode(const ufbx_scene *scene, const ufbx_node *node, scenegraph::SceneGraph &sceneGraph,
					  int parent) const;
	bool saveRecursiveNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
						   const core::String &filename, io::SeekableWriteStream &stream, uint32_t sentinelLength);

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"FBX", {"fbx"}, {}, VOX_FORMAT_FLAG_MESH | FORMAT_FLAG_SAVE | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
