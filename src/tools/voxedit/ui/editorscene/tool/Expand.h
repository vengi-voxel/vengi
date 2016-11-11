#pragma once

#include "voxel/polyvox/RawVolume.h"

namespace voxedit {
namespace tool {

extern voxel::RawVolume* expand(const voxel::RawVolume* source, int size = 1);

}
}
