/**
 * @file
 */

#pragma once

#include "MetaMap.h"
#include "voxelformat/VolumeCache.h"

namespace rma {

LevelVolumes buildMap(const MetaMap *metaMap, const voxelformat::VolumeCachePtr &volumeCache, unsigned int seed = 0u);

} // namespace rma
