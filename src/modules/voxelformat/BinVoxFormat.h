/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/FileStream.h"
#include "io/File.h"
#include "core/String.h"

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
	int _tx = 0;
	int _ty = 0;
	int _tz = 0;
	float _scale = 0.0f;

	bool readData(const io::FilePtr& file, const size_t offset, VoxelVolumes& volumes);
	bool readHeader(const core::String& header);
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
