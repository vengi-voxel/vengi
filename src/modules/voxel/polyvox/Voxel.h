#pragma once

#include "MaterialDensityPair.h"

namespace voxel {

// density 0 - 255 (8 bits)
// material types 0 - 255 (8 bits)
typedef uint8_t VoxelType;
typedef MaterialDensityPair88 Voxel;

}
