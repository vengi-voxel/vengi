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
	bool saveMesh(const voxel::Mesh& mesh, const io::FilePtr& file) override;
};
}
