/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/File.h"
#include "core/String.h"

namespace voxel {
/**
 * @brief Wavefront Object
 */
class OBJFormat : public MeshExporter {
private:
	void writeMtlFile(const core::String& mtlName) const;
public:
	bool saveMesh(const voxel::Mesh& mesh, const io::FilePtr& file, float scale, bool quad) override;
};
}
