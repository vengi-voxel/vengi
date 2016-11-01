/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"

namespace voxel {

/**
 * @brief MagicaVoxel vox format load and save functions
 *
 * https://github.com/ephtracy/voxel-model.git
 */
class VoxFormat : public VoxFileFormat {
public:
	RawVolume* load(const io::FilePtr& file) override;
	bool save(const RawVolume* volume, const io::FilePtr& file) override;
};

}
