/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/FileStream.h"
#include <string>
#include <map>

namespace voxel {

/**
 * @brief MagicaVoxel vox format load and save functions
 *
 * https://github.com/ephtracy/voxel-model.git
 * https://voxel.codeplex.com/wikipage?title=Sample%20Codes
 */
class VoxFormat : public VoxFileFormat {
private:
	bool readAttributes(std::map<std::string, std::string>& attributes, io::FileStream& stream) const;
	bool saveAttributes(const std::map<std::string, std::string>& attributes, io::FileStream& stream);
public:
	VoxelVolumes loadGroups(const io::FilePtr& file) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
