/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/FileStream.h"

namespace voxel {

/**
 * @brief VoxEdit (Sandbox) (vxr)
 */
class VXRFormat : public VoxFileFormat {
private:
	bool loadChildVXM(const core::String& vxrPath, VoxelVolumes& volumes);
	bool importChild(const core::String& vxrPath, io::FileStream& stream, VoxelVolumes& volumes, uint32_t version);
	bool importChildOld(io::FileStream& stream, uint32_t version);
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
