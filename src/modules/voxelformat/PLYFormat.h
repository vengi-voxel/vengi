/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/File.h"

namespace voxel {
/**
 * @brief Polygon File Format or Stanford Triangle Format
 */
class PLYFormat : public MeshExporter {
public:
	bool saveMeshes(const Meshes& meshes, const io::FilePtr& file, float scale, bool quad, bool withColor, bool withTexCoords) override;
};
}
