/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"
#include "core/io/File.h"
#include <string>

namespace voxel {

/**
 * @brief BinVox (binvox) format.
 *
 * https://www.patrickmin.com/binvox/binvox.html
 */
class BinVoxFormat : public VoxFileFormat {
private:
	uint32_t _version = 0u;
	uint32_t _w = 0u;
	uint32_t _h = 0u;
	uint32_t _d = 0u;
	uint32_t _size = 0u;
	float _tx = 0.0f;
	float _ty = 0.0f;
	float _tz = 0.0f;
	float _scale = 0.0f;

	bool readData(const io::FilePtr& file, const size_t offset, VoxelVolumes& volumes);
	bool readHeader(const std::string& header);
public:
	VoxelVolumes loadGroups(const io::FilePtr& file) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
