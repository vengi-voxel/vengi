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
	bool writeRLE(io::WriteStream &stream, int rleCount, voxel::Voxel &voxel) const;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& volumes) override;
	bool saveGroups(const SceneGraph& volumes, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
