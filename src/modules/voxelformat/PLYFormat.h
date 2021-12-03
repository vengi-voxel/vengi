/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {
/**
 * @brief Polygon File Format or Stanford Triangle Format
 */
class PLYFormat : public MeshExporter {
public:
	bool saveMeshes(const Meshes& meshes, const core::String &filename, io::SeekableWriteStream& stream, float scale, bool quad, bool withColor, bool withTexCoords) override;
};
}
