/**
 * @file
 */

#pragma once

#include "MeshExporter.h"

namespace voxel {
/**
 * @brief Wavefront Object
 */
class OBJFormat : public MeshExporter {
private:
	bool writeMtlFile(const core::String& mtlName, const core::String &paletteName) const;
public:
	bool saveMeshes(const Meshes& meshes, const core::String &filename, io::SeekableWriteStream& stream, const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;
	/**
	 * @brief Voxelizes the input mesh
	 */
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
};
}
