/**
 * @file
 */

#pragma once

#include "MeshFormat.h"

namespace voxelformat {

/**
 * @brief Autodesk FBX
 * https://banexdevblog.wordpress.com/2014/06/23/a-quick-tutorial-about-the-fbx-ascii-format/
 * https://code.blender.org/2013/08/fbx-binary-file-format-specification/
 * https://github.com/libgdx/fbx-conv/
 */
class FBXFormat : public MeshFormat {
private:
	bool saveMeshesBinary(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
						  const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords);
	bool saveMeshesAscii(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
						 const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords);

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const SceneGraph &sceneGraph, const Meshes &meshes,
					const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override;
};

} // namespace voxelformat
