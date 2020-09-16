/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/File.h"

namespace voxel {
/**
 * @brief Wavefront Object
 */
class OBJFormat : public MeshExporter {
public:
	bool saveMesh(const voxel::Mesh& mesh, const io::FilePtr& file) override;
};
}
