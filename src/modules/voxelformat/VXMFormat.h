/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/FileStream.h"

namespace voxel {

/**
 * @brief VoxEdit (Sandbox) (vmx)
 */
class VXMFormat : public VoxFileFormat {
private:
	bool writeRLE(io::FileStream &stream, int rleCount, voxel::Voxel &voxel) const;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
