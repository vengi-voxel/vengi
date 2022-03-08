/**
 * @file
 */

#pragma once

#include "MeshExporter.h"

namespace voxel {

/**
 * @brief GL Transmission Format
 */
class GLTFFormat : public MeshExporter {
public:
	bool saveMeshes(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;
};

} // namespace voxel
