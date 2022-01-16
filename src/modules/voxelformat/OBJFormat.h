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
	void writeMtlFile(const core::String& mtlName, const core::String &paletteName) const;
public:
	bool saveMeshes(const Meshes& meshes, const core::String &filename, io::SeekableWriteStream& stream, float scale, bool quad, bool withColor, bool withTexCoords) override;
};
}
