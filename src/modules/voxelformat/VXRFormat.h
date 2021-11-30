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
	bool importChild(const core::String& vxrPath, io::ReadStream& stream, VoxelVolumes& volumes, uint32_t version);
	bool importChildOld(io::ReadStream& stream, uint32_t version);
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::ReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::ReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
