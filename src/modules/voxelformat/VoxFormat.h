/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"
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
	bool saveAttributes(const std::map<std::string, std::string>& attributes, io::FileStream& stream) const;
	bool saveChunk_LAYR(io::FileStream& stream, int layerId, const std::string& name, bool visible) const;
	bool saveChunk_nTRN(io::FileStream& stream, int layerId, const voxel::Region& region) const;
	bool saveChunk_XYZI(io::FileStream& stream, const voxel::RawVolume* volume, const voxel::Region& region) const;
	bool saveChunk_SIZE(io::FileStream& stream, const voxel::Region& region) const;
	bool saveChunk_RGBA(io::FileStream& stream) const;
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
