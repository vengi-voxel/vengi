/**
 * @file
 */

#include "ClientPager.h"

namespace client {

ClientPager::ClientPager(const voxelformat::VolumeCachePtr &volumeCache) :
		voxelworld::WorldPager(volumeCache) {
}

}
