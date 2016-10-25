#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "io/File.h"

namespace voxel {

/**
 * @brief MagicaVoxel vox loader
 *
 * https://github.com/ephtracy/voxel-model.git
 */
class VoxLoader {
public:
	RawVolume* load(const io::FilePtr& file);
};

}
