#pragma once

#include "core/Common.h"

DISABLE_WARNING(unused-but-set-variable,unused-but-set-variable,42)
#include <PolyVox/PagedVolume.h>
#include "Voxel.h"

namespace voxel {

typedef PolyVox::PagedVolume<Voxel> WorldData;

}
