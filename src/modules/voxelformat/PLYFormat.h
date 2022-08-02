/**
 * @file
 */

#pragma once

#include "MeshFormat.h"

namespace voxelformat {
/**
 * @brief Polygon File Format or Stanford Triangle Format
 *
 * @ingroup Formats
 */
class PLYFormat : public MeshFormat {
public:
	bool saveMeshes(const core::Map<int, int> &, const SceneGraph &, const Meshes &meshes, const core::String &filename,
					io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad, bool withColor,
					bool withTexCoords) override;
};
}
