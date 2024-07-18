/**
 * @file
 */

#pragma once

#include "MeshFormat.h"

namespace voxelformat {

/**
 * @brief Blender blend file
 *
 * @li https://gitlab.com/ldo/blendhack
 *
 * @ingroup Formats
 */
class BlendFormat : public MeshFormat {
private:
	bool loadBlend(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
				   const LoadContext &ctx, io::ReadStream &stream) const;

protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const Meshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override {
		return false;
	}
};

} // namespace voxelformat
