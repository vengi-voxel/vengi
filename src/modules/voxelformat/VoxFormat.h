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
 * https://voxel.codeplex.com/wikipage?title=Sample%20Codes
 */
class VoxFormat : public VoxFileFormat {
public:
	std::vector<RawVolume*> loadGroups(const io::FilePtr& file) override;
	bool save(const RawVolume* volume, const io::FilePtr& file) override;
};

}
