#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "io/File.h"

namespace voxel {

/**
 * @brief MagicaVoxel vox format load and save functions
 *
 * https://github.com/ephtracy/voxel-model.git
 */
class VoxFormat {
public:
	static RawVolume* load(const io::FilePtr& file);
	static bool save(const RawVolume* volume, const io::FilePtr& file);
};

}
