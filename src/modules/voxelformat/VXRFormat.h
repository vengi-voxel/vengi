/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/Stream.h"

namespace voxel {

/**
 * @brief VoxEdit (Sandbox) (vxr)
 */
class VXRFormat : public VoxFileFormat {
private:
	bool loadChildVXM(const core::String& vxrPath, VoxelVolumes& volumes);
	bool importChild(const core::String& vxrPath, io::SeekableReadStream& stream, VoxelVolumes& volumes, uint32_t version);
	bool importChildOld(io::SeekableReadStream& stream, uint32_t version);
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
