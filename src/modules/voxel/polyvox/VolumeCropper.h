#pragma once

#include "RawVolume.h"

namespace voxel {

extern RawVolume* cropVolume(const RawVolume* volume, const Voxel& emptyVoxel);

}
