/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief VoxEdit (Sandbox) (vmx)
 */
class VXMFormat : public Format {
private:
	bool writeRLE(io::WriteStream &stream, int rleCount, voxel::Voxel &voxel, uint8_t emptyColorReplacement) const;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
